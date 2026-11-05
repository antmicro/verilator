// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain.
// SPDX-FileCopyrightText: 2026 Antmicro
// SPDX-License-Identifier: CC0-1.0

module sub;
  int z;
endmodule

module m;
  if (1) begin : g
    sub s ();
  end
  class C;
    function int get();
      return g.s.z;
    endfunction
  endclass
  C c = new;
endmodule

module t;
  m u1 (), u2 ();
endmodule
