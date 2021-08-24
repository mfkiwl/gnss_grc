/*
 * Copyright 2021 Free Software Foundation, Inc.
 *
 * This file is part of GNU Radio
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

/***********************************************************************************/
/* This file is automatically generated using bindtool and can be manually edited  */
/* The following lines can be configured to regenerate this file during cmake      */
/* If manual edits are made, the following tags should be modified accordingly.    */
/* BINDTOOL_GEN_AUTOMATIC(0)                                                       */
/* BINDTOOL_USE_PYGCCXML(0)                                                        */
/* BINDTOOL_HEADER_FILE(acquisition.h)                                        */
/* BINDTOOL_HEADER_FILE_HASH(370cd407337dd6e5eea5b4366dec5466)                     */
/***********************************************************************************/

#include <pybind11/complex.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace py = pybind11;

#include <gnss/acquisition.h>
// pydoc.h is automatically generated in the build directory
#include <acquisition_pydoc.h>

void bind_acquisition(py::module &m)
{

     using acquisition = gr::gnss::acquisition;

     py::class_<acquisition, gr::block, gr::basic_block,
                std::shared_ptr<acquisition>>(m, "acquisition", D(acquisition))

         .def(py::init(&acquisition::make),
              py::arg("a_sampleFreq"),
              py::arg("a_channelNum"),
              D(acquisition, make))

         ;
}
