// DESCRIPTION: Verilator: Concrete model values for quantified membership
//
// This file ONLY is placed under the Creative Commons Public Domain
// SPDX-FileCopyrightText: 2026 The Verilator Authors
// SPDX-License-Identifier: CC0-1.0

module t;
  class C;
    rand bit [3:0] data[];
    rand bit [3:0] needle;
    rand bit yes;
    bit [3:0] search;
    constraint shape { data.size() inside {[1:3]}; }
    constraint c {
      foreach (data[i]) data[i] == 5;
      needle == search;
      if (needle inside {data}) yes == 1; else yes == 0;
      (needle inside {data}) == yes;
      !(4'h8 inside {data});
    }
  endclass
  initial begin
    automatic C c = new;
    c.data = new[2];
    foreach (c.data[i]) c.data[i] = 5;
    for (int i = 0; i < 3; i++) begin
      c.search = (i == 1) ? 8 : 5;
      if (c.randomize() != 1 || c.yes != (c.search == 5)) $fatal;
    end
    $display("*-* All Finished *-*");
    $finish;
  end
endmodule
