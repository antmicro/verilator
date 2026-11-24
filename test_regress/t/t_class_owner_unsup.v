// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain.
// SPDX-FileCopyrightText: 2026 Wilson Snyder
// SPDX-License-Identifier: CC0-1.0

package pkg;
  class reader #(type T);
    static function int get();
      return T::count;
    endfunction
  endclass
endpackage

module owner;
  int value;
  class C;
    static int count;
    function int get();
      return value;
    endfunction
  endclass
  C obj;
endmodule

module t;
  owner first (), second ();
  typedef type(first.obj) class_t;
  class_t obj;
  initial begin
`ifdef STATIC
    $display("%0d", pkg::reader#(type(first.obj))::get());
`else
    obj = new;
`endif
  end
endmodule
