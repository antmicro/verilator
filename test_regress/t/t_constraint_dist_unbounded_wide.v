// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain.
// SPDX-FileCopyrightText: 2026 OpenAI
// SPDX-License-Identifier: CC0-1.0

class W;
  rand bit [63:0] a;
  rand bit [64:0] b;
  rand bit signed [64:0] c;
  rand bit [64:0] weighted;
  bit signed [64:0] lo, hi;
  rand bit signed [64:0] runtime_range;
  rand bit split;
  constraint pick {
    runtime_range dist {[lo:hi] := 1};
    split dist {0 := 65'h1_0000_0000_0000_0000, 1 := 65'h1_0000_0000_0000_0000};
    a dist {[64'd0:$] :/ 1};
    b dist {[65'd0:$] := 1};
    c dist {[65'sh1_0000_0000_0000_0000:$] :/ 1};
    weighted dist {65'h0 := 1, [65'h1_ffff_ffff_ffff_fffc:$] := 1};
  }
endclass
module t;
  initial begin
    W w;
    int a0, a1, b0, b1, c0, c1, singleton;
    bit [3:0] seen;
    bit [4:0] seen_runtime;
    int split_count;
    w = new;
    w.lo = -2;
    w.hi = 2;
    repeat (200) begin
      if (w.randomize() != 1) $fatal(1, "unexpected UNSAT");
      if (w.runtime_range < w.lo || w.runtime_range > w.hi) $fatal;
      seen_runtime[int'(w.runtime_range) + 2] = 1;
      split_count += int'(w.split);
      if (w.a[63]) a1++; else a0++;
      if (w.b[64]) b1++; else b0++;
      if (w.c[64]) c1++; else c0++;
      if (w.weighted == '0) singleton++;
      else if (w.weighted >= 65'h1_ffff_ffff_ffff_fffc) seen[w.weighted[1:0]] = 1;
      else $fatal;
    end
    if (seen_runtime != '1 || split_count < 50 || split_count > 150) $fatal;
    $display("a=%0d/%0d b=%0d/%0d c=%0d/%0d single=%0d high=%0h", a0,a1,b0,b1,c0,c1,singleton,seen);
    if (a0 < 50 || a1 < 50 || b0 < 50 || b1 < 50 || c0 < 50 || c1 < 50 ||
        singleton < 15 || singleton > 75 || seen != '1) $fatal;
    $display("*-* All Finished *-*");
    $finish;
  end
endmodule
