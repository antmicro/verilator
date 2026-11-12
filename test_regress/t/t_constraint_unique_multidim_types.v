// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain.
// SPDX-FileCopyrightText: 2026 Antmicro
// SPDX-License-Identifier: CC0-1.0

class C;
  rand bit [3:0] a[3][2];
  rand bit [3:0] d[2][];
  rand bit [3:0] q[2][$];
  rand bit [3:0] m[2][int];
  constraint c {
    unique {a};
    unique {d};
    unique {q};
    unique {m};
    foreach (a[i, j]) a[i][j] < 6;
    foreach (d[i, j]) d[i][j] < 4;
    foreach (q[i, j]) q[i][j] < 3;
    foreach (m[i, j]) m[i][j] < 2;
  }
  function new;
    foreach (d[i]) d[i] = new[2];
    foreach (q[i]) q[i] = '{0};
    foreach (m[i]) m[i][i*5] = 0;
  endfunction
endclass
module t;
  initial begin
    automatic C c = new;
    automatic bit [15:0] seen;
    repeat (5) begin
      if (c.randomize() != 1) $stop;
      seen = 0; foreach (c.a[i, j]) begin if (seen[c.a[i][j]]) $stop; seen[c.a[i][j]] = 1; end
      seen = 0; foreach (c.d[i, j]) begin if (seen[c.d[i][j]]) $stop; seen[c.d[i][j]] = 1; end
      seen = 0; foreach (c.q[i, j]) begin if (seen[c.q[i][j]]) $stop; seen[c.q[i][j]] = 1; end
      seen = 0; foreach (c.m[i, j]) begin if (seen[c.m[i][j]]) $stop; seen[c.m[i][j]] = 1; end
    end
    $write("*-* All Finished *-*\n");
    $finish;
  end
endmodule
