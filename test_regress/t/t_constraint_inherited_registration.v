// DESCRIPTION: Verilator: Solve inherited arrays of object handles through a base class handle
//
// This file ONLY is placed under the Creative Commons Public Domain.
// SPDX-FileCopyrightText: 2026 The Verilator Authors
// SPDX-License-Identifier: CC0-1.0

// verilog_format: off
`define stop $stop
`define checkd(gotv,expv) do if ((gotv) !== (expv)) begin $write("%%Error: %s:%0d:  got=%0d exp=%0d\n", `__FILE__,`__LINE__, (gotv), (expv)); `stop; end while(0);
// verilog_format: on

class Entry;
  rand int value;
endclass

class Base;
  rand Entry first[];
  constraint base_values {foreach (first[i]) first[i].value == i + 3;}
endclass

class Middle extends Base;
  rand Entry second[];
  constraint middle_values {foreach (second[i]) second[i].value == i + 7;}
endclass

class Leaf extends Middle;
  rand byte tag;
  constraint leaf_value {tag == 9;}
endclass

module t;
  initial begin
    Leaf leaf;
    Base base;
    int success;
    leaf = new;
    base = leaf;
    leaf.first = new[2];
    leaf.second = new[1];
    foreach (leaf.first[i]) leaf.first[i] = new;
    leaf.second[0] = new;

    // A virtual solve must register both ancestors' arrays and apply the leaf's constraint.
    success = base.randomize();
    `checkd(success, 1);
    `checkd(leaf.first[0].value, 3);
    `checkd(leaf.first[1].value, 4);
    `checkd(leaf.second[0].value, 7);
    `checkd(leaf.tag, 9);
    $write("*-* All Finished *-*\n");
    $finish;
  end
endmodule
