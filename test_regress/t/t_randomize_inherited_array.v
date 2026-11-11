// DESCRIPTION: Verilator: Solver registration for inherited arrays of class handles
// SPDX-FileCopyrightText: 2026 The Verilator Authors
// SPDX-License-Identifier: CC0-1.0

// Siblings and a grandchild must solve each inherited array element, including constraints
// overridden in one sibling. Disabling an array must freeze its elements, even when the
// constraint on that array is overridden or disabled.

// verilog_format: off
`define stop $stop
`define checkd(gotv,expv) do if ((gotv) !== (expv)) begin $write("%%Error: %s:%0d:  got=%0d exp=%0d\n", `__FILE__, `__LINE__, (gotv), (expv)); `stop; end while(0);
// verilog_format: on

class Item;
  rand bit [7:0] value;
endclass

class Base;
  rand Item items[];
  rand Item extra[];
  bit [3:0] start;

  function void pre_randomize();
    items = new[2];
    foreach (items[i]) items[i] = new;
  endfunction

  constraint extra_value {foreach (extra[i]) extra[i].value == 0;}
  constraint high {foreach (items[i]) items[i].value[7:4] == 4'h3;}
  constraint low {foreach (items[i]) items[i].value[3:0] == start + 4'(i);}
endclass

class First extends Base;
endclass

class Grandchild extends First;
endclass

class Second extends Base;
  constraint extra_value {}
  constraint high {foreach (items[i]) items[i].value[7:4] == 4'ha;}
endclass

module t;
  initial begin
    automatic First first = new;
    automatic Second second = new;
    automatic Grandchild grandchild = new;
    int ok;
    first.start = 4;
    second.start = 8;
    grandchild.start = 9;
    first.extra = new[1];
    first.extra[0] = new;
    first.extra[0].value = 8'hc7;
    first.extra.rand_mode(0);
    first.extra_value.constraint_mode(0);
    second.extra = new[1];
    second.extra[0] = new;
    second.extra[0].value = 8'h49;
    second.extra.rand_mode(0);
    ok = first.randomize();
    `checkd(ok, 1);
    ok = second.randomize();
    `checkd(ok, 1);
    ok = grandchild.randomize();
    `checkd(ok, 1);
    `checkd(grandchild.items[0].value, 8'h39);
    `checkd(grandchild.items[1].value, 8'h3a);
    `checkd(first.extra[0].value, 8'hc7);
    `checkd(second.extra[0].value, 8'h49);
    `checkd(first.items[0].value, 8'h34);
    `checkd(first.items[1].value, 8'h35);
    `checkd(second.items[0].value, 8'ha8);
    `checkd(second.items[1].value, 8'ha9);
    $display("*-* All Finished *-*");
    $finish;
  end
endmodule
