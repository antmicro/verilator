// DESCRIPTION: Verilator: SystemVerilog regression test
// SPDX-FileCopyrightText: 2026 The Verilator Authors
// SPDX-License-Identifier: CC0-1.0

module probe;
endmodule
module leaf;
endmodule
module t;
  leaf target ();
  bind target probe p ();
  bind target probe p ();
endmodule
