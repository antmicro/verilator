// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain.
// SPDX-FileCopyrightText: 2026 Antmicro
// SPDX-License-Identifier: CC0-1.0

virtual class Base;
  int seen;
  function new();
    seen = get();  // Virtual call, reads x of the instance
  endfunction
  pure virtual function int get();
endclass

module m;
  int x;
  class D extends Base;
    function int get();
      return x;
    endfunction
  endclass
  D d;
  function void make();
    d = new;
  endfunction
endmodule

module t;
  m u1 (), u2 ();
  initial begin
    #1;
    u1.x = 1;
    u2.x = 2;
    u1.make();
    u2.make();
    if (u1.d.seen != 1) $stop;
    if (u2.d.seen != 2) $stop;
    $write("*-* All Finished *-*\n");
    $finish;
  end
endmodule
