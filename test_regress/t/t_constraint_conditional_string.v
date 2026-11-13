// DESCRIPTION: Verilator: Conditional hard and soft constraint expressions
// SPDX-FileCopyrightText: 2026 The Verilator Authors
// SPDX-License-Identifier: CC0-1.0

package helper_pkg;
  function automatic int transform(int n);
    return 3 * n + 5;
  endfunction
endpackage

class sample;
  rand int hard_value;
  rand int soft_value;
  bit enabled;
  int base;

  constraint c {
    enabled ? (hard_value == helper_pkg::transform(base)) : (hard_value == 11);
    soft enabled ? (soft_value == helper_pkg::transform(base)) : (soft_value == 17);
  }
endclass

module t;
  initial begin
    automatic sample item = new;
    item.base = 7;
    item.enabled = 1;
    if (item.randomize() != 1 || item.hard_value != 26 || item.soft_value != 26) $stop;
    item.enabled = 0;
    if (item.randomize() != 1 || item.hard_value != 11 || item.soft_value != 17) $stop;
    if (item.randomize() with {soft_value == 3;} != 1 || item.soft_value != 3) $stop;
    $display("*-* All Finished *-*");
    $finish;
  end
endmodule
