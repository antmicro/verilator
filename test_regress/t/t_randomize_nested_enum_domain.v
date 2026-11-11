// DESCRIPTION: Verilator: Check enum domains when randomizing nested objects
//
// This file ONLY is placed under the Creative Commons Public Domain.
// SPDX-FileCopyrightText: 2026 The Verilator Authors
// SPDX-License-Identifier: CC0-1.0

typedef enum bit [2:0] {First = 2, Last = 5} choice_t;

class Leaf;
  rand choice_t choice;
  rand bit restrict_choice;
  constraint c { if (restrict_choice) choice == Last; }
endclass

class Base;
  rand bit enabled;
  constraint base_c { enabled == 1; }
endclass

class Pair extends Base;
  rand Leaf a, b;
  function new();
    a = new;
    b = new;
  endfunction
endclass

module t;
  initial begin
    Pair pair;
    pair = new;
    repeat (20) begin
      if (pair.randomize() with {
            a.restrict_choice == 0;
            b.restrict_choice == 0;
            a.choice != b.choice;
          } == 0) $stop;
      if (!pair.enabled) $stop;
      if (!(pair.a.choice inside {First, Last}) || !(pair.b.choice inside {First, Last})
          || pair.a.choice == pair.b.choice) $stop;
    end
    $display("*-* All Finished *-*");
    $finish;
  end
endmodule
