// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain.
// SPDX-FileCopyrightText: 2026 Antmicro
// SPDX-License-Identifier: CC0-1.0

module y(input x);
  initial #1 begin
    if (x !== 1) $stop;
    $write("*-* All Finished *-*\n");
    $finish;end
endmodule

module t;
  logic [2:0] x;
  y h(x[0]);
  initial x = 3;
endmodule
