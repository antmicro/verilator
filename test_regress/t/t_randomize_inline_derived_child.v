// DESCRIPTION: Verilator: Solve derived nested fields in inline randomization through a base handle
//
// This file ONLY is placed under the Creative Commons Public Domain.
// SPDX-FileCopyrightText: 2026 The Verilator Authors
// SPDX-License-Identifier: CC0-1.0

// verilog_format: off
`define stop $stop
`define checkd(gotv,expv) do if ((gotv) !== (expv)) begin $write("%%Error: %s:%0d:  got=%0d exp=%0d\n", `__FILE__,`__LINE__, (gotv), (expv)); `stop; end while(0);
// verilog_format: on

class Leaf;
  rand bit [7:0] data;
endclass

class Base;
  rand bit [7:0] value;
endclass

class Derived extends Base;
  rand Leaf child;
  bit replace_child = 1;
  constraint coupled {child.data == value;}

  function void pre_randomize();
    if (replace_child) child = new;
  endfunction
endclass

module t;
  function automatic void check_solve(Base base, bit [7:0] expected, int expected_success = 1);
    int success;
    success = base.randomize() with {value == local:: expected;};
    `checkd(success, expected_success);
    if (success == 1) begin
      `checkd(base.value, expected);
    end
  endfunction

  initial begin
    Derived obj;
    Leaf original;
    obj = new;

    // The derived constraint must use the child created by pre_randomize.
    check_solve(obj, 8'd42);
    `checkd(obj.child.data, 8'd42);
    original = obj.child;

    // The next call must update the replacement child and preserve the old one.
    check_solve(obj, 8'd73);
    `checkd(obj.child != original, 1'b1);
    `checkd(obj.child.data, 8'd73);
    `checkd(original.data, 8'd42);

    // A disabled field on the live child becomes a state value for the solve.
    obj.replace_child = 0;
    obj.child.data.rand_mode(0);
    obj.value = 0;
    check_solve(obj, 8'd73);
    `checkd(obj.child.data, 8'd73);

    // A conflicting inline constraint must fail without changing either field.
    check_solve(obj, 8'd74, 0);
    `checkd(obj.value, 8'd73);
    `checkd(obj.child.data, 8'd73);

    obj.child.data.rand_mode(1);
    check_solve(obj, 8'd75);
    `checkd(obj.child.data, 8'd75);

    $write("*-* All Finished *-*\n");
    $finish;
  end
endmodule
