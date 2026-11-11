// DESCRIPTION: Verilator: Preserve inherited random enum ranges
//
// This file ONLY is placed under the Creative Commons Public Domain.
// SPDX-FileCopyrightText: 2026 The Verilator Authors
// SPDX-License-Identifier: CC0-1.0

module t;
  typedef enum {
    Small,
    Large
  } size_t;

  class Parent;
    rand size_t size_sel;
    rand int count;
    constraint counts {
      if (size_sel == Small) count == 1;
      if (size_sel == Large) count == 2;
    }
  endclass

  class Child extends Parent;
    bit require_three;
    constraint request {if (require_three) count == 3;}
  endclass

  initial begin
    Child child_obj;
    child_obj = new;
    // A child must retain the domain of an enum referenced only in a parent
    // constraint. A count of three would require an undeclared enum value.
    if (child_obj.randomize() with {count == 2;} != 1) $stop;
    if (child_obj.size_sel != Large || child_obj.count != 2) $stop;
    if (child_obj.randomize() with {count == 3;} != 0) $stop;
    child_obj.require_three = 1;
    if (child_obj.randomize() != 0) $stop;
    // Disabling the parent constraint permits the count, but the enum must
    // still be one of its two declared values.
    child_obj.counts.constraint_mode(0);
    if (child_obj.randomize() != 1) $stop;
    if (!(child_obj.size_sel inside {Small, Large}) || child_obj.count != 3) $stop;
    $write("*-* All Finished *-*\n");
    $finish;
  end
endmodule
