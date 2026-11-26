// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain.
// SPDX-FileCopyrightText: 2026 Antmicro Ltd
// SPDX-License-Identifier: CC0-1.0

`timescale 1ns / 1ns

// 'always @*' with timing controls waits on the variables it reads
module t;
  bit clk;
  always #1 clk = ~clk;

  // Path without a timing control
  bit enable, request;
  always @(*) begin
    request = 0;
    if (enable) request = #1 1;
  end

  // Variable written by the process is not a trigger while the process runs
  bit en2, x, req2;
  int n2;
  always @(*) begin
    n2++;
    if (en2) #1 req2 = x;
  end

  // Identifier only in an event control is not in the sensitivity list
  bit d, q;
  int n3;
  always @(*) begin
    n3++;
    @(posedge clk) q = d;
  end

  initial begin
    #2 enable = 1;
    x = 1;
    #2 if (!request) $stop;
    enable = 0;
    #1 if (request) $stop;

    en2 = 1;
    #2 en2 = 0;
    #2 if (n2 != 3 || !req2) $stop;

    d = 1;
    #10 if (n3 != 1 || !q) $stop;

    $write("*-* All Finished *-*\n");
    $finish;
  end
endmodule
