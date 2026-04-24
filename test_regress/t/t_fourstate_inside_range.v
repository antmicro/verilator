// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain.
// SPDX-FileCopyrightText: 2026 Antmicro
// SPDX-License-Identifier: CC0-1.0

module t;
  logic [3:0] value;
  int hit;

  task automatic check(input logic [3:0] in, input int expected);
    value = in;
    hit = 0;
    unique case (value) inside
      [4'd3 : 4'd10]: hit = 1;
      default: hit = 2;
    endcase
    if (hit != expected) $stop;
  endtask

  initial begin
    check(4'd5, 1);
    check(4'd2, 2);
    check(4'bx101, 2);
    check(4'bz101, 2);
    $write("*-* All Finished *-*\n");
    $finish;
  end
endmodule
