// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain, for
// any use, without warranty, 2026 by Antmicro.
// SPDX-License-Identifier: CC0-1.0

typedef struct packed {
  logic [31:0] arr1;
} St1;

typedef struct packed {
  St1 [31:0] arr2;
} St2;

module m;
  St2 st2;
  initial begin
    force st2.arr2[0].arr1 = '0;
    release st2.arr2[0].arr1;
    $finish;
  end
endmodule
