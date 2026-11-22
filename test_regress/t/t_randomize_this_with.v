// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain
// SPDX-FileCopyrightText: 2025 Antmicro
// SPDX-License-Identifier: CC0-1.0

// verilog_format: off
`define stop $stop
`define checkd(gotv,expv) do if ((gotv) !== (expv)) begin $write("%%Error: %s:%0d:  got=%0d exp=%0d\n", `__FILE__,`__LINE__, (gotv), (expv)); `stop; end while(0);
// verilog_format: on

class Cls;
  rand int v_rand;
  int v_norand;
  rand byte values[];
  task body;
    int x;
    v_norand = 42;
    x = this.randomize() with {v_rand == 17; values.size() == 3;};
    `checkd(x, 1);
    `checkd(v_rand, 17);
    `checkd(values.size(), 3);
    `checkd(v_norand, 42);
  endtask

  function void size_other(Cls other);
    int ok;
    v_norand = 5;
    other.v_norand = 11;
    ok = other.randomize() with {values.size() == local::v_norand;};
    `checkd(ok, 1);
    `checkd(other.values.size(), 5);
    `checkd(values.size(), 3);
  endfunction
endclass

module t;
  initial begin
    automatic Cls c = new;
    automatic Cls other = new;
    c.body();
    c.size_other(other);
    $finish;
  end
endmodule
