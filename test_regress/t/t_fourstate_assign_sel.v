// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain.
// SPDX-FileCopyrightText: 2026 Antmicro
// SPDX-License-Identifier: CC0-1.0

module t;
  logic [15:0] x = 2;

  initial begin
    if (x !== 16'b0000000000000010) $stop;
    x[$c("0")] = 1;
    if (x !== 16'b0000000000000011) $stop;
    x[$c("3")+:2] = 2'd3;
    if (x !== 16'b0000000000011011) $stop;
    x[$c("3")+:2] = 2'bxz;
    if (x !== 16'b00000000000xz011) $stop;
    x[$c("10")-:2] = 2'bxz;
    if (x !== 16'b00000xz0000xz011) $stop;
    x[$c("100")-:2] = 2'bxz;
    if (x !== 16'b00000xz0000xz011) $stop;
    $write("*-* All Finished *-*\n");
    $finish;
  end
endmodule
