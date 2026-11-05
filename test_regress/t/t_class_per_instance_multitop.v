// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain.
// SPDX-FileCopyrightText: 2026 Antmicro
// SPDX-License-Identifier: CC0-1.0

module m;
  int x;
  class C;
    function int get();
      return x;
    endfunction
  endclass
  C c = new;
endmodule

// Two top modules, each with one instance of m
module t;
  m u1 ();
  initial begin
    u1.x = 1;
    #2;
    if (u1.c.get() != 1) $stop;
    $write("*-* All Finished *-*\n");
    $finish;
  end
endmodule

// verilator lint_off MULTITOP
module t2;
  // verilator lint_on MULTITOP
  m u2 ();
  initial begin
    u2.x = 2;
    #1;
    if (u2.c.get() != 2) $stop;
  end
endmodule
