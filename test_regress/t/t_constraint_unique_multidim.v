// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain
// SPDX-FileCopyrightText: 2026 Verilator Authors
// SPDX-License-Identifier: CC0-1.0

class packet;
  rand bit [1:0] grid[2][2];
  constraint distinct_elements { unique {grid}; }
endclass

module t;
  typedef enum {FIRST = 2, LAST = 5} kind_t;
  bit [1:0] values[kind_t][];
  bit seen[4];

  initial begin
    automatic packet p = new;
    if (p.randomize() != 1) $stop;
    foreach (p.grid[i,j]) begin
      if (seen[p.grid[i][j]]) $stop;
      seen[p.grid[i][j]] = 1;
    end
    if (p.randomize() with {grid[0][0] == grid[1][0];} != 0) $stop;

    seen = '{default:0};
    values[FIRST] = new[2];
    values[LAST] = new[2];
    if (std::randomize(values) with {unique {values};} != 1) $stop;
    foreach (values[i,j]) begin
      if (seen[values[i][j]]) $stop;
      seen[values[i][j]] = 1;
    end
    values[LAST] = new[3];
    if (std::randomize(values) with {unique {values};} != 0) $stop;
    $write("*-* All Finished *-*\n");
    $finish;
  end
endmodule
