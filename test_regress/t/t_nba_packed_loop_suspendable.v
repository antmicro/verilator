// DESCRIPTION: Verilator: Check packed nonblocking writes from a suspendable loop
//
// This file ONLY is placed under the Creative Commons Public Domain.
// SPDX-FileCopyrightText: 2026 Wilson Snyder
// SPDX-License-Identifier: CC0-1.0

module t;
  bit clk;
  bit [255:0] bits_q;
  bit [255:0] bits_d;
  bit [255:0] previous;
  int limit;

  initial forever begin
    @(posedge clk);
    if (limit == 3) bits_q <= '1;
    foreach (bits_q[i])
      if (i < limit) bits_q[i] <= bits_d[i];
    if (limit == 3) bits_q[2:1] <= 2'b01;
    if (bits_q !== previous) $fatal(1, "Early NBA update");
  end

  initial begin
    bit [255:0] expected;
    bits_q = '1;
    previous = bits_q;
    bits_d = {8{32'h5432_dcba}};
    limit = 77;
    #1;
    clk = 1;
    expected = '1;
    expected[76:0] = bits_d[76:0];
    #1;
    if (bits_q !== expected) $fatal(1, "First update: %h expected %h", bits_q, expected);
    previous = bits_q;
    clk = 0;
    limit = 33;
    bits_d = ~bits_d;
    #1;
    clk = 1;
    expected[32:0] = bits_d[32:0];
    #1;
    if (bits_q !== expected) $fatal(1, "Second update: %h expected %h", bits_q, expected);
    previous = bits_q;
    clk = 0;
    limit = 3;
    bits_d = '0;
    #1;
    clk = 1;
    #1;
    if (bits_q !== {{253{1'b1}}, 3'b010})
      $fatal(1, "Overlapping updates: %h", bits_q);
    $display("*-* All Finished *-*");
    $finish;
  end
endmodule
