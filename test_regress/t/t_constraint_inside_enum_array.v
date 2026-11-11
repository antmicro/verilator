// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain.
// SPDX-FileCopyrightText: 2026 The Verilator Authors
// SPDX-License-Identifier: CC0-1.0

module t;
  // verilator lint_off WIDTHTRUNC
  typedef enum bit [2:0] {
    A = 1,
    B = 3
  } key_t;
  class packet;
    key_t choices[$] = {A, B};
    rand bit [7:0] value;
    constraint in_choices {value inside {choices};}
  endclass
  packet p;
  initial begin
    p = new;
    if (!p.randomize() with {value == 3;}) $stop;
    if (p.value != 3) $stop;
    if (p.randomize() with {value == 11;}) $stop;
    $write("*-* All Finished *-*\n");
    $finish;
  end
endmodule
