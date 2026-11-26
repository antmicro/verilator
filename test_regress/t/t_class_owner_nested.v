// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain.
// SPDX-FileCopyrightText: 2026 Antmicro
// SPDX-License-Identifier: CC0-1.0

interface inner (
    input int x
);
  class C;
    function int get();
      return x;  // Input port
    endfunction
  endclass
  C c = new;
endinterface

interface outer;
  int x;
  inner in (.x(x));
endinterface

module t;
  // One inner in each of several parents
  outer o1 (), o2 ();
  virtual outer v;
  initial begin
    o1.x = 1;
    o2.x = 2;
    v = o2;
    #1;
    if (o1.in.c.get() !== 1) $stop;
    if (o2.in.c.get() !== 2) $stop;
    if (v.in.c.get() !== 2) $stop;
    o1.x = 11;
    o2.x = 12;
    #1;
    if (o1.in.c.get() !== 11) $stop;
    if (v.in.c.get() !== 12) $stop;
    $write("*-* All Finished *-*\n");
    $finish;
  end
endmodule
