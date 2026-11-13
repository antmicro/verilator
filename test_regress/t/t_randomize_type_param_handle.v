// DESCRIPTION: Verilator: SystemVerilog regression test
// SPDX-FileCopyrightText: 2026 The Verilator Authors
// SPDX-License-Identifier: CC0-1.0

class item;
  rand int x;
endclass
class base_seq #(type ITEM_T = item);
endclass
class seq extends base_seq #(item);
  function int apply_value(ITEM_T it);
    return it.randomize() with {x == 42;};
  endfunction
endclass
module t;
  initial begin
    automatic item it = new;
    automatic seq s = new;
    it.x = -1;
    if (s.apply_value(it) != 1 || it.x != 42) $fatal(1, "inherited type parameter");
    $display("*-* All Finished *-*");
    $finish;
  end
endmodule
