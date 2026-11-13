// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain
// SPDX-FileCopyrightText: 2026 Antmicro
// SPDX-License-Identifier: CC0-1.0

// verilog_format: off
`define stop $stop
`define checkd(gotv,expv) do if ((gotv) !== (expv)) begin $write("%%Error: %s:%0d:  got=%0d exp=%0d\n", `__FILE__,`__LINE__, (gotv), (expv)); `stop; end while(0);
// verilog_format: on

typedef int int_arr_t[];

package config_pkg;
  typedef enum bit [3:0] {
    First = 1,
    Second = 4,
    Third = 7,
    Fourth = 10
  } op_t;
  typedef op_t table_t[$];
  table_t choices;
endpackage

class Packet;
  rand int unsigned index;
  rand config_pkg::op_t op;
  constraint lookup {
    index < config_pkg::choices.size();
    op == config_pkg::choices[index];
  }
endclass

class Wide;
  typedef bit [68:0] word_t;
  word_t values[];
  bit enabled = 1;
  rand int index;
  rand word_t val;
  constraint lookup {
    enabled -> val == values[index];
    val == (enabled ? values[index] : 69'd42);
  }
endclass

class C;
  rand int x;
  rand bit [1:0] foo;
endclass

class Cls4;
  int_arr_t arr;
  task body();
    C c;
    c = new;
    arr = new[2];
    arr[0] = 123;
    arr[1] = 124;
    repeat (40) begin
      if (c.randomize() with {
            solve foo before x;
            x == arr[bit'(foo)];
            foo <= 1;
          } != 1)
        $stop;
      `checkd(c.x, arr[bit'(c.foo)]);
    end
  endtask
endclass

module t;
  import config_pkg::*;
  initial begin
    automatic Cls4 c4 = new;
    automatic Packet p = new;
    automatic Wide w = new;
    automatic int index = -1;
    int ok;
    c4.body();

    choices = '{Third, First, Fourth};
    ok = p.randomize() with {op == First;};
    `checkd(ok, 1);
    `checkd(p.index, 1);
    ok = p.randomize() with {index == 2;};
    `checkd(ok, 1);
    `checkd(p.op, Fourth);
    ok = p.randomize() with {op == Second;};
    `checkd(ok, 0);
    if (choices != table_t'{Third, First, Fourth}) $stop;
    choices = '{First, Second};
    ok = p.randomize() with {op == First;};
    `checkd(ok, 1);
    `checkd(p.index, 0);
    if (choices != table_t'{First, Second}) $stop;
    ok = std::randomize(index) with {choices[index] == Second;};
    `checkd(ok, 1);
    `checkd(index, 1);

    w.values = '{69'h1_0000_0000_0000_0011, 69'h10_0000_0000_0000_0022};
    ok = w.randomize() with {index == 1;};
    `checkd(ok, 1);
    if (w.val != w.values[1]) $stop;
    ok = w.randomize() with {index == -1;};
    `checkd(ok, 0);
    ok = w.randomize() with {index == 2;};
    `checkd(ok, 0);
    w.values.delete();
    ok = w.randomize() with {index == 0;};
    `checkd(ok, 0);
    w.values = '{69'd1};
    w.enabled = 0;
    ok = w.randomize() with {
      index == -1;
      val == 69'd42;
    };
    `checkd(ok, 1);
    if (w.val != 69'd42) $stop;

    $write("*-* All Finished *-*\n");
    $finish;
  end
endmodule
