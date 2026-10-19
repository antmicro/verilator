// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain.
// SPDX-FileCopyrightText: 2026 Antmicro
// SPDX-License-Identifier: CC0-1.0

module m;
  int x;
  if (1) begin : g
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
    #1;
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
