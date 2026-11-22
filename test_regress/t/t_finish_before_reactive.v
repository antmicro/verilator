// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain.
// SPDX-FileCopyrightText: 2026 Tao Liu
// SPDX-License-Identifier: CC0-1.0

module t;
  bit clk, first, disabled;
  int assertion_edges;
  initial #8 disabled = 1;
  assert property (@(posedge clk) disable iff (disabled) 1) ++assertion_edges;
  always #1 clk = !clk;
  always @(posedge clk) begin
    if (first) $finish;
    first = 1;
  end
  final begin
    if (assertion_edges != 1) $fatal(1, "Reactive assertion saw %0d rising edges", assertion_edges);
    $write("*-* All Finished *-*\n");
  end
endmodule
