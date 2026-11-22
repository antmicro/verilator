// DESCRIPTION: Verilator: VPI deposits on a continuously driven input
//
// This file ONLY is placed under the Creative Commons Public Domain.
// SPDX-FileCopyrightText: 2026 The Verilator Authors
// SPDX-License-Identifier: CC0-1.0

module observer(
    input wire clk,
    input logic a /* verilator public_flat_rw */,
    input wire b /* verilator public_flat_rw */,
    output bit [1:0] sampled
);
  always @(posedge clk) sampled <= {a, b};
endmodule

module t;
  import "DPI-C" context function int deposit(input string path, input int value);
  import "DPI-C" context function int read_value(input string path);

  bit clk;
  bit [1:0] switches;
  wire [1:0] sampled;
  observer dut(clk, &switches, |switches, sampled);
  always #5 clk = ~clk;

  initial begin
    switches = 2'b01;
    #7;
    if (deposit("t.dut.a", 1) == 0 || deposit("t.dut.b", 0) == 0) $stop;
    // Both reduction drivers keep their values as their operands change.
    #1 switches = 2'b10;
    #8;
    if (read_value("t.dut.a") != 1 || read_value("t.dut.b") != 0 || sampled !== 2'b10) $stop;
    switches = 2'b11;
    #10 switches = 0;
    #10 switches = 2'b11;
    #10;
    if (sampled !== 2'b11) $stop;
    $display("*-* All Finished *-*");
    $finish;
  end
endmodule
