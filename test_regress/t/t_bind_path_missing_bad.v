// DESCRIPTION: Verilator: SystemVerilog regression test
// SPDX-FileCopyrightText: 2026 The Verilator Authors
// SPDX-License-Identifier: CC0-1.0

module probe;
endmodule
module leaf;
endmodule
module t;
  for (genvar i = 0; i < 2; i++) begin : gen_leaf
    leaf inst ();
  end
  bind gen_leaf[3].inst probe p ();
endmodule
