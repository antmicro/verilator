// DESCRIPTION: Verilator: SystemVerilog regression test
// SPDX-FileCopyrightText: 2026 The Verilator Authors
// SPDX-License-Identifier: CC0-1.0

package objects;
  class registry #(type T = int);
    static function T make();
      T obj = new;
      return obj;
    endfunction
  endclass
  class item;
    typedef registry#(item) type_id;
    function int depth();
      return this.item.size();
    endfunction
    int item[$];
  endclass
endpackage
module t;
  initial begin
    automatic objects::item obj = objects::item::type_id::make();
    obj.item.push_back(1);
    if (obj.depth() != 1) $fatal;
    $display("*-* All Finished *-*");
    $finish;
  end
endmodule
