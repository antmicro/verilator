// DESCRIPTION: Verilator: SystemVerilog regression test
// SPDX-FileCopyrightText: 2026 The Verilator Authors
// SPDX-License-Identifier: CC0-1.0

class item;
  rand int x, y;
  function int apply_value(int x);
    return this.randomize() with {x == local::x;};
  endfunction
  function int apply_both(int want_x, int want_y);
    return randomize(x, y) with {x == local::want_x; y == local::want_y;};
  endfunction
endclass
module t;
  initial begin
    automatic item it = new;
    it.x = -1;
    if (it.apply_value(42) != 1 || it.x != 42) $fatal(1, "local scope lookup failed");
    if (it.apply_both(11, 24) != 1 || it.x != 11 || it.y != 24) $fatal(1, "implicit this");
    $display("*-* All Finished *-*");
    $finish;
  end
endmodule
