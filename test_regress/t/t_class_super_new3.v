// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain, for
// any use, without warranty, 2025 by Antmicro.
// SPDX-License-Identifier: CC0-1.0

class Base;
  function new(int x);
  endfunction
endclass
class Derived extends Base;
  function new;
    int x = 1;
    super.new(x);
  endfunction
endclass

module t;
  initial begin
    Derived d = new;
    $write("*-* All Finished *-*\n");
    $finish;
  end
endmodule
