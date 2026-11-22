// DESCRIPTION: Verilator: Check bounds on randomized indices into fixed state arrays
//
// This file ONLY is placed under the Creative Commons Public Domain.
// SPDX-FileCopyrightText: 2026 The Verilator Authors
// SPDX-License-Identifier: CC0-1.0

// verilog_format: off
`define stop $stop
`define checkd(gotv,expv) do if ((gotv) !== (expv)) begin $write("%%Error: %s:%0d:  got=%0d exp=%0d\n", `__FILE__,`__LINE__, (gotv), (expv)); `stop; end while(0);
// verilog_format: on

typedef enum {
  DIRECT,
  SLICED,
  GUARDED,
  MATRIX
} Mode;

class Lookup;
  bit [7:0] entries[3] = '{11, 22, 33};
  bit [7:0] grid[-2:0][5:3];
  rand int key, column;
  rand bit [7:0] value;
  Mode mode;

  constraint lookup {
    if (mode == DIRECT) value == entries[key];
    if (mode == SLICED) value == entries[key[1:0]];
    if (mode == GUARDED) {
      if (key >= 0 && key < 3)
      value == entries[key];
      else
      value == 90;
    }
    if (mode == MATRIX) value == grid[key][column];
  }

  function new();
    foreach (grid[i, j]) grid[i][j] = 8'(10 + (i + 2) * 3 + j - 3);
  endfunction

  function void check(Mode which, int row, int col, bit [7:0] wanted, int expected);
    int success;
    mode = which;
    success = this.randomize() with {
      key == local:: row;
      column == local:: col;
      value == local:: wanted;
    };
    `checkd(success, expected);
    if (success == 1) begin
      `checkd(key, row);
      `checkd(column, col);
      `checkd(value, wanted);
    end
  endfunction
endclass

module t;
  bit [64:0] wide_index;
  bit [7:0] random_values[3];
  bit [7:0] entries[3] = '{11, 22, 33};
  initial begin
    Lookup obj;
    obj = new;
    // A wide index cannot wrap into range. An explicit two-bit slice can.
    obj.check(DIRECT, 5, 0, 22, 0);
    obj.check(DIRECT, -1, 0, 0, 0);
    obj.check(SLICED, 5, 0, 22, 1);
    obj.check(SLICED, -3, 0, 22, 1);
    obj.check(SLICED, 3, 0, 0, 0);

    // An unused out-of-bounds select must allow the other branch to solve.
    obj.check(GUARDED, -1, 0, 90, 1);

    // Check both dimension offsets, their directions and their bounds.
    obj.check(MATRIX, -1, 4, 14, 1);
    obj.check(MATRIX, -3, 4, 0, 0);
    obj.check(MATRIX, -1, 6, 0, 0);
    if (std::randomize(wide_index) with {
      wide_index == 65'h1_00000000_00000001;
      entries[wide_index] == 22;
    } != 0) $stop;
    if (std::randomize(wide_index) with {
      wide_index == 65'h1_00000000_00000001;
      entries[wide_index[1:0]] == 22;
    } != 1) $stop;
    if (std::randomize(wide_index, random_values) with {
      wide_index == 65'h1_00000000_00000001;
      random_values[wide_index] == 22;
    } != 0) $stop;
    if (std::randomize(wide_index, random_values) with {
      wide_index == 1;
      random_values[wide_index] == 22;
    } != 1 || random_values[1] != 22) $stop;
    $write("*-* All Finished *-*\n");
    $finish;
  end
endmodule
