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
using output_type = double;
nav_decoding::sptr nav_decoding::make(int channelNum, float _sampleFreq) {
  return gnuradio::make_block_sptr<nav_decoding_impl>(channelNum, _sampleFreq);
}

/*
 * The private constructor
 */
nav_decoding_impl::nav_decoding_impl(int channelNum, float _sampleFreq)
    : gr::sync_decimator(
          "nav_decoding",
          gr::io_signature::make(1 /* min inputs */, 1 /* max inputs */, sizeof(input_type)),
          gr::io_signature::make(1 /* min outputs */, 1 /*max outputs */, sizeof(output_type)),
          500 /*<+decimation+>*/),
      sampleFreq{_sampleFreq} {
  set_tag_propagation_policy(TPP_DONT);
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
  iterator = 0;
  result = 0;
  absSampleCount = 0;
  gatherNavBits = true;
}

int nav_decoding_impl::work(int noutput_items, gr_vector_const_void_star &input_items,
                            gr_vector_void_star &output_items) {
  const input_type *in = reinterpret_cast<const input_type *>(input_items[0]);
  output_type *out = reinterpret_cast<output_type *>(output_items[0]);

  const uint64_t nread = this->nitems_read(0); // number of items read on port 0
  const size_t ninput_items = noutput_items * 500;
  // read all tags associated with port 0 for items in this work function
  tags.clear();
  this->get_tags_in_range(tags, 0, nread, nread + ninput_items);

  for (int j = 0; j < noutput_items; j++) {
    if (gatherNavBits)
      std::cout << "Iterator   " << iterator / 1000 << std::endl;
    towCounter += 500;
    if (absSampleCount != 0) {
      absSampleCount = pmt::to_uint64(tags.at(j * 500 + subStartIndex).value);
    }

    for (int i = j * 500; i < j * 500 + 500; i++) {

      if (in[i] == 0 && gatherNavBits) {
        absSampleCount = 0;
        iterator = 0;
        subframeStart = 0;
      }

      if (i != 0 && in[i - 1] != 0 && in[i] == 0 && gatherNavBits) {
        std::cout << "data extraction restarted for PRN   " << PRN
                  << "      iterator at:   " << (double)iterator / 1000 << std::endl;
      }

      if (i != 0 && in[i - 1] == 0 && in[i] != 0 && gatherNavBits) {
        std::cout << "data extraction restarted for PRN   " << PRN << std::endl;
      }

      // Check if Channel PRN has been changed
      if (tags.size() == ninput_items) {

        int receivedPRN{};
        try {
          receivedPRN = std::stoi(pmt::symbol_to_string(tags.at(i).key));
        } catch (const std::exception &e) {
          std::cout << "Bad PRN received from Tag in nav_decoding_impl.cc" << std::endl;
          std::cerr << e.what() << '\n';
        }

        if (receivedPRN != 0 && PRN != receivedPRN && in[i] != 0) {
          std::cout << "Data extraction restarted,   old PRN:   " << PRN
                    << "    new PRN:   " << receivedPRN << std::endl;
          restartDataExtraction();
          PRN = receivedPRN;
        }
      }

      if (gatherNavBits && in[i] != 0) {
        if (iterator < subframeStart + 1500 * 20 - 1) {

          in[i] > 0 ? buffer[iterator] = 1 : buffer[iterator] = -1;

          // Find the start of a Sub-frame to generate correct nav-bits
          if (iterator == samplesForPreamble - 1) {
            std::deque<int> temp(buffer, buffer + samplesForPreamble);
            subframeStart = findSubframeStart(temp);
            std::cout << "Subframe start for PRN:    " << PRN
                      << "   identified:   " << subframeStart << std::endl;
          }
        }
        if (iterator == (subframeStart + 1500 * 20 - 1)) {
          towCounter = 0;
          subStartIndex = i % 500;
          absSampleCount = pmt::to_uint64(tags.at(j * 500 + subStartIndex).value);
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

          std::cout << "Nav bits for PRN   " << PRN << "sent to nav_solution" << std::endl;
          gatherNavBits = false;
        }

        iterator++;
      }

      if (absSampleCount != 0) {
        const size_t item_index = j; // which output item gets the tag?
        const uint64_t offset = this->nitems_written(0) + item_index;
        tag_t tag;
        tag.offset = offset;
        tag.key = pmt::mp("towoffset");
        tag.value = pmt::from_double((towCounter * 1.0) / 1000);
        this->add_item_tag(0, tag);
      }
    }

    result = (double)absSampleCount / (sampleFreq / 1000.0);
    out[j] = result ? result : 0;
  }
  return noutput_items;
}

} /* namespace gnss */
} /* namespace gr */
