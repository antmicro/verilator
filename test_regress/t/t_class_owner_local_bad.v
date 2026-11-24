// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain.
// SPDX-FileCopyrightText: 2026 Antmicro
// SPDX-License-Identifier: CC0-1.0

module m;
  class C;
    local static int n;
    protected static function int f();
      return 1;
    endfunction
  endclass
  initial begin
    C::n = 1;
    $display("%0d", C::f());
  end
endmodule

module t;
  m u1 (), u2 ();
endmodule
