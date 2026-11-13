// DESCRIPTION: Verilator: Foreach distribution draws in sibling constraint branches
// SPDX-FileCopyrightText: 2026 The Verilator Authors
// SPDX-License-Identifier: CC0-1.0

class packet;
  rand int a[2];
  rand int b[2];
  bit mode;
  constraint values {
    foreach (a[i]) {
      if (mode) a[i] dist {1 := 2, 2 := 1};
      else a[i] dist {3 := 1, 4 := 2};
      b[i] dist {5 := 2, 6 := 1};
    }
  }
endclass

module t;
  initial begin
    automatic packet p = new;
    repeat (8) begin
      p.mode = !p.mode;
      if (p.randomize() != 1) $fatal;
      foreach (p.a[i]) begin
        if (p.mode && !(p.a[i] inside {1, 2})) $fatal;
        if (!p.mode && !(p.a[i] inside {3, 4})) $fatal;
        if (!(p.b[i] inside {5, 6})) $fatal;
      end
    end
    $display("*-* All Finished *-*");
    $finish;
  end
endmodule
