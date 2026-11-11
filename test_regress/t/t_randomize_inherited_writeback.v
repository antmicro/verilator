// DESCRIPTION: Verilator: Preserve solved child fields when a class inherits a constraint
//
// This file ONLY is placed under the Creative Commons Public Domain.
// SPDX-FileCopyrightText: 2026 The Verilator Authors
// SPDX-License-Identifier: CC0-1.0

// verilog_format: off
`define stop $stop
`define checkd(gotv,expv) do if ((gotv) !== (expv)) begin $write("%%Error: %s:%0d:  got=%0d exp=%0d\n", `__FILE__,`__LINE__, (gotv), (expv)); `stop; end while(0);
// verilog_format: on

class Item;
  rand bit [6:0] value, other;
endclass

class Parent;
  rand Item item = new;
  bit [6:0] expected;
  constraint match_value {item.value == expected;}
endclass

class Derived extends Parent;
endclass

module t;
  initial begin
    Parent parent;
    Derived derived;
    int success;
    bit [6:0] first_other;
    bit other_changed;
    parent = new;
    derived = new;

    // A derived solve must preserve the constrained field while the free field still varies.
    for (int i = 0; i < 8; ++i) begin
      parent.expected = 7'(i);
      success = parent.randomize();
      `checkd(success, 1);
      `checkd(parent.item.value, parent.expected);

      derived.expected = ~parent.expected;
      success = derived.randomize();
      `checkd(success, 1);
      `checkd(derived.item.value, derived.expected);
      if (i == 0) first_other = derived.item.other;
      other_changed |= derived.item.other != first_other;
    end
    `checkd(other_changed, 1'b1);

    $write("*-* All Finished *-*\n");
    $finish;
  end
endmodule
