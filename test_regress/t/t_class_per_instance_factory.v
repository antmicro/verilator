// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain.
// SPDX-FileCopyrightText: 2026 Antmicro
// SPDX-License-Identifier: CC0-1.0

package reg_pkg;
  class registry #(
      type T = int
  );
    static function T make();
      T t = new;
      return t;
    endfunction
  endclass
endpackage

interface ifc;
  int x;
  class C;
    function int get();
      return x;
    endfunction
  endclass
  typedef reg_pkg::registry#(C) reg_t;
endinterface

module t;
  ifc i1 (), i2 ();
  initial begin
    i1.x = 1;
    i2.x = 2;
    // Static method of a specialization with C of each instance
    if (i1.reg_t::make().get() != 1) $stop;
    if (i2.reg_t::make().get() != 2) $stop;
    $write("*-* All Finished *-*\n");
    $finish;
  end
endmodule
