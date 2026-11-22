// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain.
// SPDX-FileCopyrightText: 2026 Tao Liu
// SPDX-License-Identifier: CC0-1.0

module t;
  timeunit 1ns;
  timeprecision 1ps;

  bit clk;
  bit rst = 1;
  bit action_disable;
  int live_edges, sampled_edges, action_edges;
  bit first_nba, second_nba;

  always #1 clk = !clk;
  always @(posedge clk) rst <= !rst;
  always @(negedge clk) action_disable = 0;

  // The disable reads the NBA value. The property operand reads its pre-edge value.
  assert property (@(posedge clk) disable iff (rst) rst)
    live_edges |= 1 << (int'($time) / 2);
  else $fatal;

  assert property (@(posedge clk) disable iff ($sampled(rst)) !rst)
    sampled_edges |= 1 << (int'($time) / 2);
  else $fatal;

  // All disables have been evaluated before a pass action can change one.
  assert property (@(posedge clk) 1) action_disable = 1;
  assert property (@(posedge clk) disable iff (action_disable) 1)
    action_edges |= 1 << (int'($time) / 2);

  function void sample_previous();
    second_nba <= first_nba;
  endfunction

  // Both right-hand sides see the value from before the action-block NBAs.
  assert property (@(posedge clk) disable iff (rst) 1) first_nba <= !first_nba;
  assert property (@(posedge clk) disable iff (rst) 1) sample_previous();

  initial begin
    #2;
    if (!first_nba || second_nba) $fatal;
    #4;
    if (first_nba || !second_nba) $fatal;
    #2;
    if (live_edges != 'b0101 || sampled_edges != 'b1010 || action_edges != 'b1111)
      $fatal(1, "live=%b sampled=%b action=%b", live_edges[3:0], sampled_edges[3:0], action_edges[3:0]);
    $write("*-* All Finished *-*\n");
    $finish;
  end
endmodule
