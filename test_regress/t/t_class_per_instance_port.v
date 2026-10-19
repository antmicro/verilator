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

module consumer (
    ifc p
);
  initial p.c = new;  // Constructed in the interface on the port
  function int get();
    return p.c.get();
  endfunction
endmodule

module t;
  ifc i1 (), i2 ();
  consumer u1 (i1), u2 (i2);
  initial begin
    i1.x = 1;
    i2.x = 2;
    #1;
    if (u1.get() !== 1) $stop;
    if (u2.get() !== 2) $stop;
    $write("*-* All Finished *-*\n");
    $finish;
  end
endmodule
