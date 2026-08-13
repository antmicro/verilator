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
    int ok;
    A a;
    a = new;
    arr = new[2];
    arr[1] = 123;
    ok = a.randomize() with {x == arr[1];};
    `checkd(ok, 1);
    `checkd(a.x, 123);
  endtask
endclass

class B;
  rand int x;
  rand int arr[];
endclass

class Cls2;
  task body();
    int ok;
    B b;
    b = new;
    b.arr = new[2];
    b.x = 1;
    b.arr[1] = 2;
    ok = b.randomize() with {x == arr[1];};
    `checkd(ok, 1);
    `checkd(b.x, b.arr[1]);
  endtask
endclass

class Cls3;
  rand int arr[];
  task body();
    int ok;
    B b;
    b = new;
    b.arr = new[2];
    b.x = 1;
    b.arr[1] = 2;
    arr = new[2];
    arr[1] = 3;
    ok = b.randomize() with {x == arr[1];};
    `checkd(ok, 1);
    `checkd(b.x, b.arr[1]);
    `checkd(arr[1], 3);
  endtask
endclass

class C;
  rand int x;
  rand bit foo;
endclass

class Cls4;
  int arr[];
  task body();
    C c;
    c = new;
    arr = new[2];
    arr[0] = 123;
    arr[1] = 124;
    if (c.randomize() with {solve foo before x; x == arr[foo]; foo <= 1;} != 1) $stop;
    `checkd(c.x, arr[c.foo]);
  endtask
endclass

module t;
  Cls c;
  Cls2 c2;
  Cls3 c3;
  Cls4 c4;
  initial begin
    c = new;
    c2 = new;
    c3 = new;
    c4 = new;
    c.body();
    c2.body();
    c3.body();
    c4.body();
    $write("*-* All Finished *-*\n");
    $finish;
  end
endmodule
