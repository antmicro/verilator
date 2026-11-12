// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain.
// SPDX-FileCopyrightText: 2026 <TODO>
// SPDX-License-Identifier: CC0-1.0

class Item;
  rand bit [9:0] value;
  rand bit [72:0] wide;
  bit set_before_solve;
  int pre_count, post_count;
  function void pre_randomize();
    pre_count++;
    if (set_before_solve) wide = 73'h1_ace01234_56789abc;
  endfunction
  function void post_randomize();
    post_count++;
  endfunction
endclass

class Derived extends Item;
  rand bit [8:0] extra;
endclass

class Leaf;
  rand bit [4:0] value;
  bit impossible;
  int pre_count, post_count;
  constraint legal { value == 3; if (impossible) value == 4; }
  function void pre_randomize();
    pre_count++;
  endfunction
  function void post_randomize();
    post_count++;
  endfunction
endclass

class Parent;
  rand Leaf a, b;
  rand bit [4:0] tag;
  constraint legal { tag == 12; }
  function new();
    a = new;
    b = new;
  endfunction
endclass

module t;
  initial begin
    Derived d;
    Item i;
    Parent p;
    Leaf a, b;
    int ok;

    d = new;
    i = d;
    p = new;
    a = p.a;
    b = p.b;

    ok = i.randomize() with { value == 7; wide == 73'h123_456789ab_cdef0123; };
    if (ok != 1 || i.value != 7 || i.wide != 73'h123_456789ab_cdef0123) $stop;
    d.extra = 91;
    i.set_before_solve = 1;
    ok = i.randomize() with { value == 0; value == 1; };
    if (ok != 0 || i.value != 7 || i.wide != 73'h1_ace01234_56789abc || d.extra != 91) $stop;
    if (i.pre_count != 2 || i.post_count != 1) $stop;
    ok = i.randomize() with { value == 9; wide == 73'h12f_abcdef00_11223344; };
    if (ok != 1 || i.value != 9 || i.wide != 73'h12f_abcdef00_11223344) $stop;
    if (i.pre_count != 3 || i.post_count != 2) $stop;

    p.tag = 22;
    a.value = 17;
    b.value = 18;
    a.impossible = 1;
    ok = p.randomize();
    if (ok != 0 || p.tag != 22 || a.value != 17 || b.value != 18) $stop;
    if (a.pre_count != 1 || b.pre_count != 1 || a.post_count != 0 || b.post_count != 0) $stop;
    a.impossible = 0;
    ok = p.randomize();
    if (ok != 1 || p.tag != 12 || a.value != 3 || b.value != 3) $stop;
    if (a.pre_count != 2 || b.pre_count != 2 || a.post_count != 1 || b.post_count != 1) $stop;
    if (p.a != a || p.b != b) $stop;

    $write("*-* All Finished *-*\n");
    $finish;
  end
endmodule
