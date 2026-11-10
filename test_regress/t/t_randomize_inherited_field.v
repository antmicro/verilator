// DESCRIPTION: Verilator: SystemVerilog regression test
// SPDX-FileCopyrightText: 2026 The Verilator Authors
// SPDX-License-Identifier: CC0-1.0

package items;
  class base_item;
    rand int x;
  endclass
  class item extends base_item;
  endclass
endpackage
package seqs;
  class seq;
    function int apply_value(items::item it, int value);
      return it.randomize() with {x == local::value;};
    endfunction
  endclass
endpackage
module t;
  initial begin
    automatic items::item it = new;
    automatic seqs::seq s = new;
    it.x = -1;
    if (s.apply_value(it, 42) != 1 || it.x != 42) $fatal(1, "inherited property lookup failed");
    $display("*-* All Finished *-*");
    $finish;
  end
endmodule
