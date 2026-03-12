#!/usr/bin/env python3
# DESCRIPTION: Verilator: Verilog Test driver/expect definition
#
# This program is free software; you can redistribute it and/or modify it
# under the terms of either the GNU Lesser General Public License Version 3
# or the Perl Artistic License Version 2.0.
# SPDX-FileCopyrightText: 2024 Wilson Snyder
# SPDX-License-Identifier: LGPL-3.0-only OR Artistic-2.0

import vltest_bootstrap

test.scenarios('vlt')
test.top_filename = "t/t_uvm_dpi.v"
test.pli_filename = "t/t_uvm_1_2/src/dpi/uvm_dpi.cc"

if test.have_dev_gcov:
    test.skip("Test suite intended for full dev coverage without needing this test")

test.compile(v_flags2=[
    "--binary", "--vpi", "-j 0", "-Wno-UNSIGNED", "+incdir+t/t_uvm_1_2/src", test.pli_filename
])

test.execute(expect_filename=test.golden_filename)

test.passes()
