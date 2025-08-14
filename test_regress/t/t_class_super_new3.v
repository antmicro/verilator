// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain, for
// any use, without warranty, 2025 by Antmicro.
// SPDX-License-Identifier: CC0-1.0

class Base;
  int j;
  function new(int x, int y);
    j = x + y;
  endfunction
  function int get();
    return j;
  endfunction
endclass
class Derived extends Base;
  function new(int y);
    int x = 1;
    int z = y;
    super.new(x, z);
  endfunction
endclass

module t;
  initial begin
    Derived d = new(7);
    if (d.get() != 8) $stop;
    $write("*-* All Finished *-*\n");
    $finish;
  end
endmodule
