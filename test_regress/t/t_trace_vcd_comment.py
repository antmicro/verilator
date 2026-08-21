#!/usr/bin/env python3
# DESCRIPTION: Verilator: Verilog Test driver/expect definition
#
# This program is free software; you can redistribute it and/or modify it
# under the terms of either the GNU Lesser General Public License Version 3
# or the Perl Artistic License Version 2.0.
# SPDX-FileCopyrightText: 2026 Wilson Snyder
# SPDX-License-Identifier: LGPL-3.0-only OR Artistic-2.0

import vltest_bootstrap

test.scenarios('vlt_all')
test.top_filename = "t_trace_cat.v"

test.compile(make_top_shell=False,
             make_main=False,
             v_flags2=["--trace-vcd --exe", test.pli_filename])

test.execute()

vcd = test.obj_dir + "/simcomment.vcd"

test.file_grep(vcd, r'\$comment comment before first dump \$end\n')
test.file_grep(vcd, r'\$comment single line comment at t=3 \$end\n')
test.file_grep(vcd, r'\$comment multi line comment at t=5\nsecond line \$end\n')

test.passes()
