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
for defines in ([], ['+define+UNRELATED_VIF']):
    test.compile(verilator_flags2=['--binary', *defines])
    if test.vlt_all:
        header = test.glob_one(test.obj_dir + '/*effect_owner__03a__03aWriter__Vclpkg.h')
        test.file_grep_count(header, r'effect_owner\* __PVT____Vcapture[0-9]+p;', 1)
        test.file_grep_count(header, r'Writer__Vclpkg\* __PVT____Vcapture[0-9]+p;', 1)
    test.execute()
test.passes()
