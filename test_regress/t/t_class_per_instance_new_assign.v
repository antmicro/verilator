// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain.
// SPDX-FileCopyrightText: 2026 Antmicro
// SPDX-License-Identifier: CC0-1.0

interface ifc;
  int x;
  class C;
    function int get();
      return x;
    endfunction
  endclass
  C c;
endinterface

module leaf;
  int x;
  class C;
    function int get();
      return x;
    endfunction
  endclass
  C c;
endmodule

module mid;
  leaf l ();
endmodule

module t;
  mid a (), b ();
  ifc i1 (), i2 (), i3 (), i4 ();
  virtual ifc v;
  virtual ifc vs[2];
  initial begin
    a.l.x = 1;
    b.l.x = 2;
    // Through a hierarchical path
    a.l.c = new;
    b.l.c = new;
    if (a.l.c.get() != 1) $stop;
    if (b.l.c.get() != 2) $stop;
    i1.x = 11;
    i2.x = 12;
    i3.x = 13;
    i4.x = 14;
    // Through a virtual interface
    v = i2;
    v.c = new;
    if (i2.c.get() != 12) $stop;
    v = i1;
    v.c = new;
    if (i1.c.get() != 11) $stop;
    // Through elements of an array of virtual interfaces
    vs[0] = i3;
    vs[1] = i4;
    vs[1].c = new;
    vs[0].c = new;
    if (i3.c.get() != 13) $stop;
    if (i4.c.get() != 14) $stop;
    $write("*-* All Finished *-*\n");
    $finish;
  end
endmodule
