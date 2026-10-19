// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain.
// SPDX-FileCopyrightText: 2026 Antmicro
// SPDX-License-Identifier: CC0-1.0

virtual class Base;
  function new();
    setup();
  endfunction
  pure virtual function void setup();
endclass

class D extends Base;
  int a;
  int b = 5;
  int c;
  function new();
    super.new();
    c = 3;
  endfunction
  function void setup();
    a = 7;
    b = 9;
    c = 9;
  endfunction
endclass

module t;
  initial begin
    D d;
    d = new;
    // IEEE 1800-2017 8.7: initialization after super.new overwrites what setup() wrote
    if (d.a != 0) $stop;
    if (d.b != 5) $stop;
    if (d.c != 3) $stop;
    $write("*-* All Finished *-*\n");
    $finish;
  end
endmodule
