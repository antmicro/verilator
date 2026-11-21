// DESCRIPTION: Verilator: Awaited sensitivities sharing trigger words and edge state
//
// This file ONLY is placed under the Creative Commons Public Domain.
// SPDX-FileCopyrightText: 2026 The Verilator Authors
// SPDX-License-Identifier: CC0-1.0

// verilog_format: off
`define stop $stop
`define checkd(gotv,expv) do if ((gotv) !== (expv)) begin $write("%%Error: %s:%0d: got=%0d exp=%0d\n", `__FILE__, `__LINE__, (gotv), (expv)); `stop; end while(0);
// verilog_format: on

`timescale 1ns/1ps
module t;
  localparam N = 129;
  bit [N-1:0] clk = '0;
  int pos_count[N], neg_count[N], change_count[N];
  int expected_pos[N], expected_neg[N], expected_change[N];
  int combined = 0, expected_combined = 0;

  // Distinct edge types share previous-value state. Independent sensitivities
  // span multiple words; a combined sensitivity also crosses word boundaries.
  for (genvar i = 0; i < N; ++i) begin
    initial forever begin
      @(posedge clk[i]);
      pos_count[i] <= pos_count[i] + 1;
    end
    initial forever begin
      @(negedge clk[i]);
      neg_count[i] <= neg_count[i] + 1;
    end
    initial forever begin
      @(clk[i]);
      change_count[i] <= change_count[i] + 1;
    end
  end
  initial forever begin
    @(posedge clk[0] or negedge clk[N-1]);
    combined <= combined + 1;
  end
  initial begin
    #1;
    for (int step = 1; step <= 257; ++step) begin
      bit [N-1:0] next_clk;
      for (int i = 0; i < N; ++i) begin
        next_clk[i] = ((step + i) % (3 + i % 7)) == 0;
        if (!clk[i] && next_clk[i]) ++expected_pos[i];
        if (clk[i] && !next_clk[i]) ++expected_neg[i];
        if (clk[i] != next_clk[i]) ++expected_change[i];
      end
      if ((!clk[0] && next_clk[0]) || (clk[N-1] && !next_clk[N-1])) ++expected_combined;
      clk = next_clk;
      #1;
      for (int i = 0; i < N; ++i) begin
        `checkd(pos_count[i], expected_pos[i]);
        `checkd(neg_count[i], expected_neg[i]);
        `checkd(change_count[i], expected_change[i]);
      end
      `checkd(combined, expected_combined);
    end
    begin
      automatic int total_pos = 0, total_neg = 0, total_change = 0;
      for (int i = 0; i < N; ++i) begin
        total_pos += pos_count[i];
        total_neg += neg_count[i];
        total_change += change_count[i];
      end
      if (total_pos == 0 || total_neg == 0 || total_change == 0 || combined == 0) `stop;
      $display("PASS pos=%0d neg=%0d changed=%0d combined=%0d checks=%0d",
               total_pos, total_neg, total_change, combined, 257 * (3 * N + 1));
    end
    $display("*-* All Finished *-*");
    $finish;
  end
endmodule
