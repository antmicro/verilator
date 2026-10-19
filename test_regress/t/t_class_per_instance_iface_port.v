// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain.
// SPDX-FileCopyrightText: 2026 Antmicro
// SPDX-License-Identifier: CC0-1.0

interface ifc;
  int v;
  int w;
  function int getv();
    return v;
  endfunction
  modport mp(input v, output w, import getv);
endinterface

module sub;
  int s;
  function int gets();
    return s;
  endfunction
endmodule

module m (
    ifc p,
    ifc.mp q
);
  sub child ();
  class C;
    int at_new;
    function new();
      at_new = p.v;
    endfunction
    function int get();
      return p.v;
    endfunction
    // Through a modport
    function int getq();
      return q.v;
    endfunction
    function void set(int a);
      p.w = a;
      q.w = a + 1;
    endfunction
    // Function of the interface
    function int getf();
      return p.getv();
    endfunction
    // Child instance
    function int getc();
      return child.s + child.gets();
    endfunction
  endclass
  C c = new;
endmodule

module t;
  ifc i1 (), i2 (), j1 (), j2 ();
  m u1 (
      .p(i1),
      .q(j1)
  );
  m u2 (
      .p(i2),
      .q(j2)
  );
  initial begin
    u1.child.s = 5;
    u2.child.s = 6;
    #1;
    i1.v = 1;
    i2.v = 2;
    j1.v = 3;
    j2.v = 4;
    #1;
    if (u1.c.get() != 1) $stop;
    if (u2.c.get() != 2) $stop;
    if (u1.c.getq() != 3) $stop;
    if (u2.c.getq() != 4) $stop;
    u1.c.set(10);
    u2.c.set(20);
    if (i1.w != 10) $stop;
    if (i2.w != 20) $stop;
    if (j1.w != 11) $stop;
    if (j2.w != 21) $stop;
    if (u1.c.getf() != 1) $stop;
    if (u2.c.getf() != 2) $stop;
    if (u1.c.getc() != 10) $stop;
    if (u2.c.getc() != 12) $stop;
    $write("*-* All Finished *-*\n");
    $finish;
  end
endmodule
