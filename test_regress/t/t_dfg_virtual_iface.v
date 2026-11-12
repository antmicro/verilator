// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain.
// SPDX-FileCopyrightText: 2026 Antmicro
// SPDX-License-Identifier: CC0-1.0

interface bus_if (
    input logic clk
);
  logic [7:0] data;
  logic [7:0] acc = 0;
  logic [7:0] sum;
  logic [7:0] mixed;
  assign sum = acc + 8'd3;
  assign mixed = sum ^ 8'h5a;
  always @(posedge clk) acc <= mixed;
endinterface

module t;
  logic clk = 0;
  always #5 clk = ~clk;
  bus_if b1 (clk), b2 (clk);
  virtual bus_if v;
  initial begin
    v = b1;
    v.data = 1;
    repeat (3) @(posedge clk);
    #1;
    if (b1.acc != 83) $stop;
    if (b2.acc != 83) $stop;
    if (b1.data != 1) $stop;
    $write("*-* All Finished *-*\n");
    $finish;
  end
endmodule
