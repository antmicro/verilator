// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain.
// SPDX-FileCopyrightText: 2026 Antmicro
// SPDX-License-Identifier: CC0-1.0

module t;
  logic a, b;
  logic [1:0] c = 2'b1x;
  assign {>>{a, b}} = c;

  initial begin
    $write("*-* All Finished *-*\n");
    $finish;
  end
endmodule
