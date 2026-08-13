// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain
// SPDX-FileCopyrightText: 2026 Antmicro
// SPDX-License-Identifier: CC0-1.0

// verilog_format: off
`define stop $stop
`define checkd(gotv,expv) do if ((gotv) !== (expv)) begin $write("%%Error: %s:%0d:  got=%0d exp=%0d\n", `__FILE__,`__LINE__, (gotv), (expv)); `stop; end while(0);
// verilog_format: on

class A;
  rand int x;
endclass

class Cls;
  rand int arr[];
  task body();
    A a;
    a = new;
    arr = new[2];
    arr[1] = 123;
    if (a.randomize() with {x == arr[1];} != 1) $stop;
    `checkd(a.x, 123);
  endtask
endclass

class B;
  rand int x;
  rand int arr[];
endclass

class Cls2;
  task body();
    B b;
    b = new;
    b.arr = new[2];
    b.x = 1;
    b.arr[1] = 2;
    if (b.randomize() with {x == arr[1];} != 1) $stop;
    if (b.x != b.arr[1]) $stop;
  endtask
endclass

module t;
Cls c;
Cls2 c2;
  initial begin
    c = new;
    c2 = new;
    c.body();
    c2.body();
    $finish;
  end
endmodule
