#!/usr/bin/env python3
# DESCRIPTION: Verilator: Verilog Test driver/expect definition
#
<<<<<<<< HEAD:test_regress/t/t_interface_virtual_unused.py
# Copyright 2024 by Wilson Snyder. This program is free software; you
# can redistribute it and/or modify it under the terms of either the GNU
========
# Copyright 2025 by Wilson Snyder. This program is free software; you can
# redistribute it and/or modify it under the terms of either the GNU
>>>>>>>> d664d9722 (alias: add linting tests for transitive property and checking widths):test_regress/t/t_lint_alias_transitive.py
# Lesser General Public License Version 3 or the Perl Artistic License
# Version 2.0.
# SPDX-License-Identifier: LGPL-3.0-only OR Artistic-2.0

import vltest_bootstrap

test.scenarios('vlt')

test.top_filename = "t/t_alias_transitive.v"

test.lint()

test.passes()
