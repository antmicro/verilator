// DESCRIPTION: Verilator: Randomize a constrained child through an inherited handle
//
// This file ONLY is placed under the Creative Commons Public Domain.
// SPDX-FileCopyrightText: 2026 The Verilator Authors
// SPDX-License-Identifier: CC0-1.0

// verilog_format: off
`define stop $stop
`define checkd(gotv,expv) do if ((gotv) !== (expv)) begin $write("%%Error: %s:%0d:  got=%0d exp=%0d\n", `__FILE__,`__LINE__, (gotv), (expv)); `stop; end while(0);
// verilog_format: on

class Child;
  rand bit [6:0] value;
  bit [6:0] expected;
  constraint match_value {value == expected;}
endclass

class Base;
endclass

class Container extends Base;
  rand Child child;
endclass

class Derived extends Container;
endclass

module t;
  initial begin
    Derived obj;
    Base base;
    Child original;
    int success;
    obj = new;
    obj.child = new;
    original = obj.child;
    base = obj;

    // Both static handle types must solve the child's constraint on the first call.
    obj.child.expected = 7'd23;
    success = obj.randomize();
    `checkd(success, 1);
    `checkd(original.value, 7'd23);

    // Replacing the inherited handle must redirect writeback to the new child.
    obj.child = new;
    obj.child.expected = 7'd91;
    success = base.randomize();
    `checkd(success, 1);
    `checkd(obj.child.value, 7'd91);
    `checkd(original.value, 7'd23);

    $write("*-* All Finished *-*\n");
    $finish;
  end
endmodule
