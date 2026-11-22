#!/usr/bin/env python3
# DESCRIPTION: Verilator: Check constructor-local initialization and base call evaluation order.
# SPDX-FileCopyrightText: 2026 The Verilator Authors
# SPDX-License-Identifier: LGPL-3.0-only OR Artistic-2.0

import vltest_bootstrap

test.scenarios('simulator')

test.compile()
test.execute()
test.passes()
