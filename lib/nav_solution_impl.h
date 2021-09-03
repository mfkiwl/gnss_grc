/* -*- c++ -*- */
/*
 * Copyright 2021 German Abdurahmanov.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef INCLUDED_GNSS_NAV_SOLUTION_IMPL_H
#define INCLUDED_GNSS_NAV_SOLUTION_IMPL_H

#include "ephemeris.h"
#include "sat-position.h"
#include <gnss/nav_solution.h>
#include <vector>

namespace gr {
namespace gnss {

class nav_solution_impl : public nav_solution {
private:
  std::vector<Ephemeris> ephemerides;
  bool startNavigation = true;
  int test = 0;
  unsigned int iterator = 0;
  int initDelay = 250;
  // The speed of light, [m/s]
  long int c = 299792458;
  float startOffset = 68.802;
  std::vector<float> pseudoRanges;

public:
  nav_solution_impl();
  ~nav_solution_impl();

  // Where all the action really happens
  int work(int noutput_items, gr_vector_const_void_star &input_items,
           gr_vector_void_star &output_items);
};

} // namespace gnss
} // namespace gr

#endif /* INCLUDED_GNSS_NAV_SOLUTION_IMPL_H */
