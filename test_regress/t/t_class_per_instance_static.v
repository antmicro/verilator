// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain.
// SPDX-FileCopyrightText: 2026 Antmicro
// SPDX-License-Identifier: CC0-1.0

package pkg;
  class reg_t #(
      type T = int
  );
    static function int call();
      return T::sget();
    endfunction
  endclass
endpackage

module single;
  class C;
    static int n;
    static function int sget();
      return n + 1;
    endfunction
  endclass
  C c = new;
endmodule

interface ifc;
  int x;
  virtual class A;
    static int an;
    pure virtual function int f();
  endclass
  class C extends A;
    static int n = 7;
    static function int sget();
      return n + x;
    endfunction
    static function int twice(int a);
      return a * 2;
    endfunction
    function int get();
      return n + C::n + this.n + sget() + C::sget();
    endfunction
    function void inc();
      n++;
    endfunction
    function int f();
      return an;
    endfunction
    function int peek(C o);
      return o.n;
    endfunction
  endclass
  C c = new;
  function void setn(int a);
    C::n = a;
    A::an = C::twice(a) * 5;
  endfunction
  function int getn();
    return C::sget();
  endfunction
  function int getan();
    return c.f();
  endfunction
  function int getpeek();
    return c.peek(c);
  endfunction
  function int getcall();
    return pkg::reg_t#(C)::call();
  endfunction
endinterface

module t;
  // With one instance, statics stay members of the class
  single u ();
  ifc i1 (), i2 ();
  virtual ifc v;
  initial begin
    u.c.n = 5;
    if (u.c.n != 5) $stop;
    if (u.c.sget() != 6) $stop;
    v = i2;
    i1.x = 100;
    i2.x = 200;
    if (i1.c.n != 7) $stop;
    if (v.c.n != 7) $stop;
    // Each instance has its own C::n and A::an
    i1.setn(1);
    v.setn(2);
    if (i1.getn() != 101) $stop;
    if (i2.getn() != 202) $stop;
    if (i1.c.get() != 1 + 1 + 1 + 101 + 101) $stop;
    if (v.c.get() != 2 + 2 + 2 + 202 + 202) $stop;
    if (i1.getan() != 10) $stop;
    if (v.getan() != 20) $stop;
    // A package class specialized with the class calls its static method
    if (i1.getcall() != 101) $stop;
    if (v.getcall() != 202) $stop;
    if (i1.getpeek() != 1) $stop;
    if (v.getpeek() != 2) $stop;
    if (v.c.sget() != 202) $stop;
    // Through a handle
    i1.c.n = 5;
    if (i1.c.n != 5) $stop;
    if (i1.getn() != 105) $stop;
    if (v.getn() != 202) $stop;
    i1.c.inc();
    if (i1.c.n != 6) $stop;
    if (v.c.n != 2) $stop;
    $write("*-* All Finished *-*\n");
    $finish;
  end
endmodule
