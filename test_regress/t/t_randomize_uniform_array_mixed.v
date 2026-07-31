// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain.
// SPDX-FileCopyrightText: 2026 Antmicro
// SPDX-License-Identifier: CC0-1.0

// Checks that randomize() with an array mixed with a plain scalar in the
// same class, linked by a conditional constraint, covers the whole solution
// space, not just a lucky subset of it. Each sample is also printed so the
// driver can check the distribution's uniformity (Jensen-Shannon divergence)
// from the samples, not just coverage.

module t;
  class C;
    rand bit [2:0] arr[2];
    rand bit sel;
    constraint c {
      if (sel) { arr[0] > arr[1]; }
      else { arr[0] <= arr[1]; }
    }
  endclass

  // Every (arr[0],arr[1]) pair in [0:7]^2 has exactly one sel value that
  // satisfies the constraint, so all 8*8 pairs are solutions.
  localparam int NUM_SOLUTIONS = 64;
  localparam int NUM_ITERS = 25 * NUM_SOLUTIONS;

  initial begin
    automatic C c = new;
    automatic int seen[int];
    automatic int distinct = 0;
    automatic int key;
    for (int i = 0; i < NUM_ITERS; ++i) begin
      if (c.randomize() != 1) begin
        $display("%%Error: randomize failed");
        $stop;
      end
      if (c.sel && !(c.arr[0] > c.arr[1])) begin
        $display("%%Error: constraint violated, sel=%0d arr=%0d,%0d", c.sel, c.arr[0], c.arr[1]);
        $stop;
      end
      if (!c.sel && !(c.arr[0] <= c.arr[1])) begin
        $display("%%Error: constraint violated, sel=%0d arr=%0d,%0d", c.sel, c.arr[0], c.arr[1]);
        $stop;
      end
      key = int'({c.sel, c.arr[0], c.arr[1]});  // 7-bit packed key, fits in int
      if (!seen.exists(key)) begin
        seen[key] = 1;
        distinct++;
      end
      $display("%0d %0d %0d", c.sel, c.arr[0], c.arr[1]);
    end
    if (distinct != NUM_SOLUTIONS) begin
      $display("%%Error: only %0d/%0d distinct combinations covered", distinct, NUM_SOLUTIONS);
      $stop;
    end
    $write("*-* All Finished *-*\n");
    $finish;
  end
endmodule
