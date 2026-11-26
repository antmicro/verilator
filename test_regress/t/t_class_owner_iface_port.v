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

module holder;
  ifc concrete ();
endmodule

module m (
    ifc p,
    ifc.mp q
);
  holder child ();
  ifc direct ();
  if (1) begin : g
    ifc inblk ();
  end
  class C;
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
    // Interface instance in a child module
    function virtual ifc getc();
      return child.concrete;
    endfunction
    function virtual ifc getd();
      return direct;
    endfunction
    function virtual ifc getg();
      return g.inblk;
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
  virtual ifc vc;
  initial begin
    u1.child.concrete.v = 5;
    u2.child.concrete.v = 6;
    u1.direct.v = 7;
    u2.direct.v = 8;
    u1.g.inblk.v = 9;
    u2.g.inblk.v = 10;
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
    vc = u1.c.getc();
    if (vc.v != 5) $stop;
    vc = u2.c.getc();
    if (vc.v != 6) $stop;
    vc = u1.c.getd();
    if (vc.v != 7) $stop;
    vc = u2.c.getd();
    if (vc.v != 8) $stop;
    vc = u1.c.getg();
    if (vc.v != 9) $stop;
    vc = u2.c.getg();
    if (vc.v != 10) $stop;
    $write("*-* All Finished *-*\n");
    $finish;
  end
endmodule
