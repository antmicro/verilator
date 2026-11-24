// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain.
// SPDX-FileCopyrightText: 2026 Antmicro
// SPDX-License-Identifier: CC0-1.0

module leaf;
  int x;
  class C;
    function int get();
      return x;
    endfunction
  endclass
  C c = new;
endmodule

module mid;
  leaf l ();
endmodule

interface inner;
  int x;
  class C;
    function int get();
      return x;
    endfunction
  endclass
  C c = new;
endinterface

interface outer;
  inner in ();
endinterface

module t;
  // One leaf and one inner in each of several parents
  mid a (), b ();
  outer o1 (), o2 ();
  virtual outer v;
  initial begin
    a.l.x = 10;
    b.l.x = 20;
    o1.in.x = 1;
    o2.in.x = 2;
    v = o2;
    #1;
    if (a.l.c.get() !== 10) $stop;
    if (b.l.c.get() !== 20) $stop;
    if (o1.in.c.get() !== 1) $stop;
    if (o2.in.c.get() !== 2) $stop;
    if (v.in.c.get() !== 2) $stop;
    $write("*-* All Finished *-*\n");
    $finish;
  end
endmodule
