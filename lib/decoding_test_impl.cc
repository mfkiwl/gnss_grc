/* -*- c++ -*- */
/*
 * Copyright 2021 German Abdurahmanov.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "decoding_test_impl.h"
#include "ephemeris.h"
#include "helper-functions.h"
#include <gnuradio/io_signature.h>
#include <pmt/pmt.h>
#include <algorithm>
#include <string>
#include <vector>


namespace gr {
namespace gnss {

using input_type = float;
using output_type = float;
decoding_test::sptr decoding_test::make(int prn)
{
    return gnuradio::make_block_sptr<decoding_test_impl>(prn);
}

/*
 * The private constructor
 */
decoding_test_impl::decoding_test_impl(int prn)
    : gr::sync_block("decoding_test",
                     gr::io_signature::make(
                         1 /* min inputs */, 1 /* max inputs */, sizeof(input_type)),
                     gr::io_signature::make(
                         1 /* min outputs */, 1 /*max outputs */, sizeof(output_type))),
      PRN(prn)
{
    message_port_register_out(pmt::string_to_symbol("result"));
    int reversePreambleShort[]{ 1, 1, 0, 1, 0, 0, 0, 1 };
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
decoding_test_impl::~decoding_test_impl() {}

void decoding_test_impl::printMessage(std::string msg)
{
    message_port_pub(pmt::string_to_symbol("result"), pmt::string_to_symbol(msg));
}

int decoding_test_impl::work(int noutput_items,
                             gr_vector_const_void_star& input_items,
                             gr_vector_void_star& output_items)
{
    const input_type* in = reinterpret_cast<const input_type*>(input_items[0]);
    output_type* out = reinterpret_cast<output_type*>(output_items[0]);


    for (int i = 0; i < noutput_items; i++) {
        // Collect enough data into a buffer to calculate Ephemerides later
        if (iterator < 37000) {
            if (in[i] > 0)
                buffer[iterator] = 1;
            else
                buffer[iterator] = -1;
        }

        // Find the start of a Sub-frame
        if (iterator == 13999) {
            travelTimeQue.assign(buffer, buffer + 14000);
            subframeStart = findSubframeStart(travelTimeQue);
            std::cout << subframeStart << std::endl;
        }

        // Find ephemerides. Min 5 subframes is required

        if (iterator == (subframeStart + 1500 * 20 - 1)) {
            // Prepare 5 sub frames worth of data in bit format
            std::vector<int> navBits;
            int sum{ 0 };
            int counter{ 0 };
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

            std::cout << "Nav Bits size: " << navBits.size() << std::endl;
            Ephemeris* ephResults = new Ephemeris(navBits);
        }

        out[i] = result;

        iterator++;
    }


    // Tell runtime system how many output items we produced.

    return noutput_items;
}

} /* namespace gnss */
} /* namespace gr */
