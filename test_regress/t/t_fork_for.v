// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain.
// SPDX-FileCopyrightText: 2026 Antmicro
// SPDX-License-Identifier: CC0-1.0

module t;
  bit [1:0] abcd;
  initial begin
    for (int i = 0; i < 2; i++) begin
      automatic int xyz = i;
      fork
        abcd[xyz] = 1'b1;
      join_none
    end
    #1;
    if (abcd != 2'b11) $stop;
    $write("*-* All Finished *-*\n");
    $finish;
  end
endmodule
