// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain, for
// any use, without warranty, 2025 by Antmicro.
// SPDX-License-Identifier: CC0-1.0

class Base;
  int j;
  function new(int x);
    j = x;
  endfunction
  function int get();
    return j;
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
    if (d.get() != 1) $stop;
    else begin
      $write("*-* All Finished *-*\n");
      $finish;
    end
  end
endmodule
