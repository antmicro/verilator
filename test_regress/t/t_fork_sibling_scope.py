#!/usr/bin/env python3
# DESCRIPTION: Check module scope after a named fork.
# SPDX-License-Identifier: LGPL-3.0-only OR Artistic-2.0

import vltest_bootstrap

test.scenarios('simulator')
test.compile(verilator_flags2=["--binary"])
test.execute()
test.passes()
