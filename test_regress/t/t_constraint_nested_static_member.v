// DESCRIPTION: Verilator: Static class properties in nested constraints
// SPDX-FileCopyrightText: 2026 The Verilator Authors
// SPDX-License-Identifier: CC0-1.0

class base;
  static int limit;
endclass

class child extends base;
  rand int value;
  constraint selected {value == base::limit;}
endclass

class group_t;
  rand child item = new;
  constraint nonnegative {item.value >= 0;}
endclass

module t;
  initial begin
    group_t g;
    g = new;
    for (int i = 1; i < 3; i++) begin
      base::limit = i * 17;
      if (g.randomize() != 1 || g.item.value != base::limit) $fatal;
    end
    $display("*-* All Finished *-*");
    $finish;
  end
endmodule
