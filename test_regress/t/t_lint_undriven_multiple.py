#!/usr/bin/env python3
# DESCRIPTION: Verilator: Verilog Test driver/expect definition
#
# This program is free software; you can redistribute it and/or modify it
# under the terms of either the GNU Lesser General Public License Version 3
# or the Perl Artistic License Version 2.0.
# SPDX-FileCopyrightText: 2026 Wilson Snyder
# SPDX-License-Identifier: LGPL-3.0-only OR Artistic-2.0

import re

import vltest_bootstrap

test.scenarios('linter')

SIZE_SMALL = 4000
# Linear scaling gives 4x, guard against superlinear growth
SIZE_LARGE = SIZE_SMALL * 4
MAX_RATIO = 11
# Ignore the ratio when scheduling is fast enough that timer noise dominates
MIN_SECONDS = 0.1


def lint_time(size):
    mdir = test.obj_dir + "/obj_" + str(size)
    test.lint(verilator_flags2=["--stats", "--no-debug-check", "-GN=" + str(size), "-Mdir", mdir])
    stats_filename = mdir + "/V" + test.name + "__stats.txt"
    stats = test.file_contents(stats_filename)
    match = re.search(r'Stage, Elapsed time \(sec\), \d+_undriven\s+(\S+)', stats)
    if not match:
        test.error("Undriven time not found in " + stats_filename)
    return float(match.group(1))


small = lint_time(SIZE_SMALL)
large = lint_time(SIZE_LARGE)
print(f"Undriven: {SIZE_SMALL} variables in {small:.3f}s, "
      f"{SIZE_LARGE} variables in {large:.3f}s ({large / small:.1f}x)")
if large > MIN_SECONDS and large > small * MAX_RATIO:
    test.error("Undriven time scaled superlinearly with variable count: " +
               f"{SIZE_SMALL} variables took {small:.3f}s, " +
               f"{SIZE_LARGE} variables took {large:.3f}s (over {MAX_RATIO}x)")

test.passes()
