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
/* BINDTOOL_HEADER_FILE(decoding_test.h)                                        */
/* BINDTOOL_HEADER_FILE_HASH(ae6af7d19c5b2e6b5f2d54e9d268b502)                     */
/***********************************************************************************/

#include <pybind11/complex.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace py = pybind11;

#include <gnss/decoding_test.h>
// pydoc.h is automatically generated in the build directory
#include <decoding_test_pydoc.h>

void bind_decoding_test(py::module& m)
{

    using decoding_test = gr::gnss::decoding_test;


    py::class_<decoding_test,
               gr::sync_block,
               gr::block,
               gr::basic_block,
               std::shared_ptr<decoding_test>>(m, "decoding_test", D(decoding_test))

        .def(py::init(&decoding_test::make),
             py::arg("channelNum"),
             py::arg("codePhase"),
             D(decoding_test, make))


        ;
}
