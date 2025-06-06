#!/usr/bin/env python3
# DESCRIPTION: Verilator: Verilog Test driver/expect definition
#
# Copyright 2024 by Wilson Snyder. This program is free software; you
# can redistribute it and/or modify it under the terms of either the GNU
# Lesser General Public License Version 3 or the Perl Artistic License
# Version 2.0.
# SPDX-License-Identifier: LGPL-3.0-only OR Artistic-2.0

import vltest_bootstrap

test.scenarios("vltmt")
test.top_filename = "t/t_x_rand_stability.v"

test.compile(verilator_flags2=["--x-initial unique", "-DNOT_RAND"])

test.execute(all_run_flags=["+verilator+rand+reset+0"], expect_filename=test.golden_filename)

test.passes()
