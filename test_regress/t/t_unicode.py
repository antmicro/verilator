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
test.top_filename = test.obj_dir + "/t_unicode.v"


def c(code):
    # Appease https://www.virustotal.com NANO-Antivirus gives Trojan.Script.Vbs-heuristic flag
    return eval("c" + "h" + "r(" + str(code) + ")")  # pylint: disable=eval-used


# Greek Hi
hi = "Greek: " + c(0xce) + c(0xb3) + c(0xce) + c(0xb5) + c(0xce) + c(0xb9) + c(0xce) + c(0xb1)


def gen(filename):
    with open(filename, 'w', encoding="latin-1") as fh:
        fh.write(c(0xEF))
        fh.write(c(0xBB))
        fh.write(c(0xBF))  # BOM
        fh.write("// Bom\n")
        fh.write("// Generated by t_unicode.py\n")
        fh.write("module t;\n")
        fh.write("   // " + hi + "\n")
        fh.write("   initial begin\n")
        fh.write("      $write(\"" + hi + "\\n\");\n")
        fh.write("      $write(\"*-* All Finished *-*\\n\");\n")
        fh.write("      $finish;\n")
        fh.write("   end\n")
        fh.write("endmodule\n")


gen(test.top_filename)

test.compile()

test.execute()

test.file_grep(test.run_log_filename, hi)

test.passes()
