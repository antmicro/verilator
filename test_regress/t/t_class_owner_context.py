#!/usr/bin/env python3
# DESCRIPTION: Verilator: Verilog Test driver/expect definition
#
# This program is free software; you can redistribute it and/or modify it
# under the terms of either the GNU Lesser General Public License Version 3
# or the Perl Artistic License Version 2.0.
# SPDX-FileCopyrightText: 2026 Wilson Snyder
# SPDX-License-Identifier: LGPL-3.0-only OR Artistic-2.0

import vltest_bootstrap

test.scenarios('simulator')
test.compile(verilator_flags2=['--binary'])
if test.vlt_all:
    # Two addressed children require two fields, shared by both owners' implementations.
    header = test.glob_one(test.obj_dir + '/*capture_pair_owner__03a__03aReader__Vclpkg.h')
    test.file_grep_count(header, r'captured_target\* __PVT____Vcapture[0-9]+p;', 2)
    symbols = test.obj_dir + '/' + test.vm_prefix + '__Syms.h'
    test.file_grep_count(symbols, r'captured_target TOP__.*capture_pairs.*__(left|right);', 4)
    test.file_grep_not(symbols, r'uncaptured_neighbor')
test.execute()
test.passes()
