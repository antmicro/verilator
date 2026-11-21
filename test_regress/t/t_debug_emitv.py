#!/usr/bin/env python3
# DESCRIPTION: Verilator: Verilog Test driver/expect definition
#
# This program is free software; you can redistribute it and/or modify it
# under the terms of either the GNU Lesser General Public License Version 3
# or the Perl Artistic License Version 2.0.
# SPDX-FileCopyrightText: 2024 Wilson Snyder
# SPDX-License-Identifier: LGPL-3.0-only OR Artistic-2.0

import re

import vltest_bootstrap

test.scenarios("vlt")

test.lint(
    # We also have dump-tree turned on, so hit a lot of AstNode*::dump() functions
    # Likewise XML
    v_flags=[
        "--lint-only",
        # --Wno-COVERIGN: cg_trans uses a goto-repetition transition bin ([->N]); the count is
        # unsupported (dropped) but the bin is still created with a non-NONE VTransRepType, which
        # is what exercises VTransRepType::ascii() and AstCoverTransItem::dump()'s repType arm.
        "--Wno-COVERIGN",
        "--bbox-unsup",  # Continue after the set-expression bin to cover its dump and emission
        "--timing",
        "--rtmd",  # Also dump the run time model descriptors
        "--dumpi-tree 9 --dumpi-V3EmitV 9 --debug-emitv",  # Dev coverage of the V3EmitV code
        "--dump-graph --dumpi-tree-json 9 --no-json-ids"
    ])

output_vs = test.glob_some(test.obj_dir + "/" + test.vm_prefix + "_*_widthcommit.tree.v")

for output_v in output_vs:
    # The C++ standard library determines these hashes. Keep repeated references
    # to each temporary distinguishable without tying the dump to one platform.
    hashes = {}

    def normalize_hash(match):
        value = match.group(2)
        return match.group(1) + str(hashes.setdefault(value, len(hashes)))

    normalized = output_v + ".normalized"
    test.file_sed(output_v, normalized, lambda s: re.sub(
        r'(__V(?:enum|seqEvent)_h)([0-9a-f]{8})(?=__\d+)', normalize_hash, s))
    test.files_identical(normalized, test.golden_filename)

if test.verbose:
    # Print if that the output Verilog is clean
    # TODO not yet round-trip clean
    test.run(
        cmd=[
            os.environ["VERILATOR_ROOT"] + "/bin/verilator",
            "--lint-only",
            output_vs[0],
        ],
        logfile=test.obj_dir + "/sim_roundtrip.log",
        fails=True,
        verilator_run=True,
    )

test.passes()
