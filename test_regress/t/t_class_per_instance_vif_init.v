// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain.
// SPDX-FileCopyrightText: 2026 Antmicro
// SPDX-License-Identifier: CC0-1.0

interface ifc;
  int x;
  class B;
    int b = x + 100;  // Initializer reads x
  endclass
  class C extends B;
    // Nested class
    class D;
      function int get();
        return x;
      endfunction
    endclass
    int y = x;
    int z;
    D d = new;
    function new();
      z = y + b;
    endfunction
  endclass
  C c;
  initial begin
    #1;
    c = new;
  end
endinterface

module t;
  ifc i1 (), i2 ();
  virtual ifc v;
  initial begin
    v = i2;
    i1.x = 1;
    i2.x = 2;
    #2;
    if (i1.c.y != 1) $stop;
    if (i1.c.b != 101) $stop;
    if (i1.c.z != 102) $stop;
    if (v.c.y != 2) $stop;
    if (v.c.b != 102) $stop;
    if (v.c.z != 104) $stop;
    if (i1.c.d.get() != 1) $stop;
    if (v.c.d.get() != 2) $stop;
    $write("*-* All Finished *-*\n");
    $finish;
  end
endmodule
