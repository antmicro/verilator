// DESCRIPTION: Verilator: SystemVerilog regression test
// SPDX-FileCopyrightText: 2026 The Verilator Authors
// SPDX-License-Identifier: CC0-1.0

package items;
  class item;
    rand int x;
  endclass
endpackage
package aliases;
  typedef items::item handle_t;
endpackage
package seqs;
  class seq;
    function int apply_value(aliases::handle_t it);
      return it.randomize() with {x == 42;};
    endfunction
  endclass
endpackage
module t;
  initial begin
    automatic items::item it = new;
    automatic seqs::seq s = new;
    it.x = -1;
    if (s.apply_value(it) != 1 || it.x != 42) $fatal(1, "package typedef");
    $display("*-* All Finished *-*");
    $finish;
  end
endmodule
