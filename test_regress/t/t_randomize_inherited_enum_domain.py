#!/usr/bin/env python3
# DESCRIPTION: Check inherited enum domains and variation with and without a base-class inline solve.
#
# This program is free software; you can redistribute it and/or modify it
# under the terms of either the GNU Lesser General Public License Version 3
# or the Perl Artistic License Version 2.0.
# SPDX-FileCopyrightText: 2026 The Verilator Authors
# SPDX-License-Identifier: LGPL-3.0-only OR Artistic-2.0

import vltest_bootstrap

test.scenarios('simulator')

if not test.have_solver:
    test.skip("No constraint solver installed")

# Compile both forms separately. The presence of an inline solve changes which
# randomization helpers are generated for the base class.
for flags in ([], ['+define+TEST_INLINE']):
    test.compile(v_flags2=flags)
    test.execute()
test.passes()
