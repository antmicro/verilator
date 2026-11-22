#!/usr/bin/env python3
# DESCRIPTION: Verilator: Verilog Test driver/expect definition
#
# This program is free software; you can redistribute it and/or modify it
# under the terms of either the GNU Lesser General Public License Version 3
# or the Perl Artistic License Version 2.0.
# SPDX-FileCopyrightText: 2026 Wilson Snyder
# SPDX-License-Identifier: LGPL-3.0-only OR Artistic-2.0

import os

import vltest_bootstrap

test.scenarios('vlt')

test.top_filename = test.obj_dir + "/" + test.name + ".v"


def generated_bytes(arms):
    with open(test.top_filename, "w", encoding="utf-8") as src:
        src.write(f"module t(input bit clk, input int unsigned weights[{arms}], output int choice);\n")
        src.write("  always @(posedge clk) randcase\n")
        for i in range(arms):
            src.write(f"    weights[{i}]: choice <= {i};\n")
        src.write("  endcase\nendmodule\n")
    test.compile(verilator_flags2=['--output-split 0'], make_main=False, verilator_make_gmake=False)
    return sum(os.path.getsize(name) for name in test.glob_some(test.obj_dir + "/*.cpp"))


small = generated_bytes(64)
large = generated_bytes(256)
# Four times as many arms should need roughly four times as much code or less.
if large > 6 * small:
    test.error(f"randcase code size grew from {small} to {large} bytes")

test.passes()
