// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain.
// SPDX-FileCopyrightText: 2026 Antmicro
// SPDX-License-Identifier: CC0-1.0

interface inf;
  logic [15:0] foo;
  modport m(output foo);
  modport s(input foo);
endinterface //inf

module m(inf.m v);
  initial begin
    #2 v.foo = 0;
    #2 v.foo = 'z;
    #2 v.foo = 'x;
    #2 v.foo = 1;
  end
endmodule

module s(inf.s v);
  initial begin
    #1 if (v.foo !== 'x) $stop;
    #2 if (v.foo !== 0) $stop;
    #2 if (v.foo !== 'z) $stop;
    #2 if (v.foo !== 'x) $stop;
    #2 if (v.foo !== 1) $stop;
    $write("*-* All Finished *-*\n");
    $finish;
  end
endmodule

module t;
  inf i();
  m p(i.m);
  s q(i.s);
endmodule
