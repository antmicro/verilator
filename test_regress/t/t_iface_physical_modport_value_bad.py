#!/usr/bin/env python3
# DESCRIPTION: Verilator: Check invalid virtual-interface constructor arguments.
#
# This program is free software; you can redistribute it and/or modify it
# under the terms of either the GNU Lesser General Public License Version 3
# or the Perl Artistic License Version 2.0.
# SPDX-FileCopyrightText: 2026 The Verilator Authors
# SPDX-License-Identifier: LGPL-3.0-only OR Artistic-2.0

import vltest_bootstrap

test.scenarios('vlt')
test.lint(fails=True, expect_filename=test.golden_filename)
test.lint(fails=True,
          verilator_flags2=['+define+BAD_MODPORT_NAME'],
          expect_filename=test.golden_filename.replace('.out', '_missing.out'))
test.passes()
