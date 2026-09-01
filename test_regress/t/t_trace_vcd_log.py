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

vcd = test.obj_dir + "/simlog.vcd"

test.file_grep(vcd, r'\$var string 1 (\S+) log \$end', '%')
test.file_grep(vcd, r'\$var string 1 (\S+) other_log \$end', '&')

test.file_grep(vcd, r'#3\n0"\nstext\\040with\\040spaces %\n')
test.file_grep(
    vcd, r'#5\n0"\nstwo\\012lines\\040and\\040a\\040\\\\ %\n'
    r'son\\040the\\040other\\040log &\n')

test.passes()
