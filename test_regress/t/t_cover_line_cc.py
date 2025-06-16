#!/usr/bin/env python3
# DESCRIPTION: Verilator: Verilog Test driver/expect definition
#
# Copyright 2024 by Wilson Snyder. This program is free software; you
# can redistribute it and/or modify it under the terms of either the GNU
# Lesser General Public License Version 3 or the Perl Artistic License
# Version 2.0.
# SPDX-License-Identifier: LGPL-3.0-only OR Artistic-2.0

import vltest_bootstrap

test.scenarios('simulator')
test.top_filename = "t/t_cover_line.v"
test.golden_filename = "t/t_cover_line.out"

test.compile(verilator_flags2=['--cc --coverage-line +define+ATTRIBUTE'])

test.execute()

test.run(cmd=[os.environ["VERILATOR_ROOT"] + "/bin/verilator_coverage",
              "--annotate-points",
              "--annotate", test.obj_dir + "/annotated",
              test.obj_dir + "/coverage.dat"],
         verilator_run=True)  # yapf:disable

test.files_identical(test.obj_dir + "/annotated/t_cover_line.v", test.golden_filename)

# Also try lcov
test.run(cmd=[os.environ["VERILATOR_ROOT"] + "/bin/verilator_coverage",
              "--write-info", test.obj_dir + "/coverage.info",
              test.obj_dir + "/coverage.dat"],
         verilator_run=True)  # yapf:disable

test.files_identical(test.obj_dir + "/coverage.info", "t/" + test.name + ".info.out")

# If installed
nout = test.run_capture("lcov --version", check=False)
version_match = re.search(r'version ([0-9.]+)', nout, re.IGNORECASE)
if not version_match:
    test.skip("lcov or genhtml not installed")

required_version = "1.14"
installed_version = version_match.group(1)

required_version_arr = required_version.split('.')
installed_version_arr = installed_version.split('.')

for i in range(len(installed_version_arr)):
    if int(installed_version_arr[i]) > int(required_version_arr[i]):
        break
    elif int(installed_version_arr[i]) == int(required_version_arr[i]):
        continue
    test.skip("lcov or genhtml too old (version " + installed_version +
              ", need version >= " + required_version)

test.run(cmd=[
    "genhtml", test.obj_dir + "/coverage.info", "--branch-coverage", "--output-directory " +
    test.obj_dir + "/html"
])

test.passes()
