#!/usr/bin/env python3
# DESCRIPTION: Verilator: Verilog Test driver/expect definition
#
# Copyright 2024 by Wilson Snyder. This program is free software; you
# can redistribute it and/or modify it under the terms of either the GNU
# Lesser General Public License Version 3 or the Perl Artistic License
# Version 2.0.
# SPDX-License-Identifier: LGPL-3.0-only OR Artistic-2.0

import vltest_bootstrap

test.scenarios('vlt_all')
test.pli_filename = "t/t_tri_gate.cpp"
test.top_filename = "t/t_tri_gate.v"

test.compile(make_top_shell=False,
             make_main=False,
             v_flags2=['+define+T_COND'],
             make_flags=['CPPFLAGS_ADD=-DT_COND'],
             verilator_flags2=["--exe", test.pli_filename, "--coverage-line"])

test.execute()

test.passes()
