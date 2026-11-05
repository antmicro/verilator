// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain.
// SPDX-FileCopyrightText: 2026 Antmicro
// SPDX-License-Identifier: CC0-1.0

module sub;
  int z;
  if (1) begin : h
    int w;
  end
  // Bound too, as sub has several instances
  class S;
  endclass
  function int getz();
    return z;
  endfunction
endmodule

module m;
  int x;
  sub s ();
  sub sa[1:0] ();
  // Same name and static member as E in the generate block
  class E;
    static int n = 1;
  endclass
  E e = new;
  if (1) begin : g
    class E;
      static int n = 2;
    endclass
    E e = new;
    int y;
    int y2;
    always_comb y2 = 2 * y;
    function int gety();
      return y;
    endfunction
    // Declared in the generate block
    class D;
      function int get();
        return 10 * x + y;
      endfunction
      // Function of the generate block
      function int getf();
        return gety();
      endfunction
    endclass
    D d = new;
  end
  for (genvar i = 0; i < 2; ++i) begin : f
    int z = i;
  end
  class C;
    // Generate block and generate loop members
    function int get();
      return g.y2 + f[1].z;
    endfunction
    function void set(int a);
      g.y = a;
    endfunction
    // Generate block function, members of a cell and its generate block
    function int getgf();
      return g.gety();
    endfunction
    function int getsub();
      return s.h.w + sa[1].z + s.getz();
    endfunction
  endclass
  C c = new;
endmodule

module t;
  m m1 (), m2 ();
  initial begin
    m1.c.set(3);
    m2.c.set(4);
    m1.x = 1;
    m2.x = 2;
    m1.s.z = 10;
    m2.s.z = 20;
    m1.s.h.w = 100;
    m2.s.h.w = 200;
    m1.sa[1].z = 1000;
    m2.sa[1].z = 2000;
    m2.e.n = 5;
    m2.g.e.n = 6;
    #1;
    if (m1.c.getgf() != 3) $stop;
    if (m2.c.getgf() != 4) $stop;
    if (m1.c.getsub() != 1110) $stop;
    if (m2.c.getsub() != 2220) $stop;
    if (m1.e.n != 1 || m1.g.e.n != 2) $stop;
    if (m2.e.n != 5 || m2.g.e.n != 6) $stop;
    if (m1.c.get() != 7) $stop;
    if (m2.c.get() != 9) $stop;
    if (m1.g.d.get() != 13) $stop;
    if (m2.g.d.get() != 24) $stop;
    if (m1.g.d.getf() != 3) $stop;
    if (m2.g.d.getf() != 4) $stop;
    $write("*-* All Finished *-*\n");
    $finish;
  end
endmodule
