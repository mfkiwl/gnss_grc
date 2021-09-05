/* -*- c++ -*- */
/*
 * Copyright 2021 German Abdurahmanov.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "nav_decoding_impl.h"
#include "ephemeris.h"
#include "helper-functions.h"
#include <algorithm>
#include <cmath>
#include <gnuradio/io_signature.h>
#include <pmt/pmt.h>
#include <string>
#include <vector>

namespace gr {
namespace gnss {

using input_type = float;
using output_type = float;
nav_decoding::sptr nav_decoding::make(int channelNum) {
  return gnuradio::make_block_sptr<nav_decoding_impl>(channelNum);
}

/*
 * The private constructor
 */
nav_decoding_impl::nav_decoding_impl(int channelNum)
    : gr::sync_decimator(
          "nav_decoding",
          gr::io_signature::make(1 /* min inputs */, 1 /* max inputs */, sizeof(input_type)),
          gr::io_signature::make(1 /* min outputs */, 1 /*max outputs */, sizeof(output_type)),
          500 /*<+decimation+>*/) {
  channel = channelNum;
  message_port_register_out(pmt::string_to_symbol("nav_bits"));

  samplesForPreamble = 14000;
  int reversePreambleShort[]{1, 1, 0, 1, 0, 0, 0, 1};
  int reversePreamble[160];
  for (int i = 0; i < 8; i++) {
    for (int n = i * 20; n < ((i + 1) * 20); n++) {
      if (reversePreambleShort[i] == 1)
        reversePreamble[n] = 1;
      else
        reversePreamble[n] = -1;
    }
  }
}

/*
 * Our virtual destructor.
 */
nav_decoding_impl::~nav_decoding_impl() {}

void nav_decoding_impl::restartDataExtraction() {
  std::cout << "data extraction restarted" << std::endl;
  iterator = 0;
  result = 0;
  travelTimeQue.clear();
  gatherNavBits = true;
}

int nav_decoding_impl::work(int noutput_items, gr_vector_const_void_star &input_items,
                            gr_vector_void_star &output_items) {
  const input_type *in = reinterpret_cast<const input_type *>(input_items[0]);
  output_type *out = reinterpret_cast<output_type *>(output_items[0]);

  const uint64_t nread = this->nitems_read(0); // number of items read on port 0
  const size_t ninput_items = noutput_items;   // assumption for sync block, this can change
  // read all tags associated with port 0 for items in this work function
  tags.clear();
  this->get_tags_in_range(tags, 0, nread, nread + ninput_items * 500);

  for (int j = 0; j < noutput_items; j++) {
    // Set PRN from the tag data and keep updating CodePhase
    if (tags.size() > 0) {
      int tagIndex = (25 * noutput_items / (j + 1)) - (25 * ninput_items - tags.size()) - 1;
      pmt::pmt_t tag_key = tags.at(tagIndex).key;
      pmt::pmt_t tag_val = tags.at(tagIndex).value;

      codePhaseMs = pmt::to_float(tag_val);
      int receivedPRN = std::stoi(pmt::symbol_to_string(tag_key));
      if (std::isnan(receivedPRN))
        continue;
      if (PRN != receivedPRN) {
        restartDataExtraction();
        PRN = receivedPRN;
      }
    }

    for (int i = j * 500; i < j * 500 + 500; i++) {
      towCounter++;

      // Add data to travel time queue and Collect enough data into a buffer
      // to prepare nav bits for Ephemeris later

      in[i] == 0  ? travelTimeQue.push_back(0)
      : in[i] > 0 ? travelTimeQue.push_back(1)
                  : travelTimeQue.push_back(-1);
      if (travelTimeQue.size() > samplesForPreamble) {
        travelTimeQue.pop_front();
      }

      // Gather Nav Bits for Ephemeris Min 5 subframes is required

      if (gatherNavBits) {
        if (iterator < subframeStart + 1500 * 20 - 1) {
          in[i] > 0 ? buffer[iterator] = 1 : buffer[iterator] = -1;
        }

        // Find the start of a Sub-frame to generate correct nav-bits
        if (iterator == samplesForPreamble - 1) {
          std::deque<int> temp(buffer, buffer + samplesForPreamble);
          subframeStart = findSubframeStart(temp);
        }

        if (iterator == (subframeStart + 1500 * 20 - 1)) {
          towCounter = 0;
          std::cout << "Ephemeris Data ready" << std::endl;
          // Prepare 5 sub frames worth of data in bit format
          std::vector<int> navBits;
          int sum{0};
          int counter{0};
          for (int i = subframeStart - 20; i < subframeStart + 1500 * 20; i++) {
            sum += buffer[i];
            counter++;
            if (counter == 20) {
              if (sum > 0)
                navBits.push_back(1);
              else
                navBits.push_back(0);
              counter = 0;
              sum = 0;
            }
          }
          auto size = sizeof(int) * navBits.size();
          auto pmt = pmt::make_blob(navBits.data(), size);
          message_port_pub(pmt::string_to_symbol("nav_bits"),
                           pmt::cons(pmt::from_long(channel), pmt));
          gatherNavBits = false;
        }
      }

      iterator++;
    }

    if (travelTimeQue.size() >= samplesForPreamble &&
        std::find(travelTimeQue.begin(), travelTimeQue.end(), 0) == travelTimeQue.end()) {
      int start = findSubframeStart(travelTimeQue);
      result = codePhaseMs + start;
      const size_t item_index = j; // which output item gets the tag?
      const uint64_t offset = this->nitems_written(0) + item_index;
      tag_t tag;
      tag.offset = offset;
      tag.key = pmt::mp("towoffset");
      tag.value = pmt::from_double((towCounter * 1.0) / 1000);
      this->add_item_tag(0, tag);
    }
    out[j] = result ? result : 0;
  }

  return noutput_items;
}

} /* namespace gnss */
} /* namespace gr */
