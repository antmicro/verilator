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

test.compile(verilator_flags2=['--assert --cc --coverage-assert'])

test.execute()

test.file_grep(test.coverage_filename, r'always_passes.*[1-9][0-9]*')
test.file_grep(test.coverage_filename, r"once_at_five.*' 1$")

test.passes()
