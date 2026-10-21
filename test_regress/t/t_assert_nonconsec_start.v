// DESCRIPTION: Verilator: Non-overlapped repetition samples the previous antecedent
//
// This file ONLY is placed under the Creative Commons Public Domain.
// SPDX-FileCopyrightText: 2026 Wilson Snyder
// SPDX-License-Identifier: CC0-1.0

module t;
  bit clk;
  bit gate = 1;
  bit data = 1;
  int match_count;
  always #5 clk = ~clk;

  // Cover actions count only nonvacuous matches of the implication.
  cover property (@(posedge clk) gate |=> data [= 2]) match_count++;

  initial begin
    @(negedge clk);
    gate = 0;
    @(negedge clk);
    data = 0;
    @(negedge clk);
    // Only one data occurrence is after the antecedent's clock. No match yet.
    if (match_count != 0) $stop;
    data = 1;
    @(negedge clk);
    data = 0;
    @(negedge clk);
    if (match_count != 1) $stop;
    $write("*-* All Finished *-*\n");
    $finish;
  end
  initial begin
    #200;
    $stop;
  end
endmodule
