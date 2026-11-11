// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain, for
// any use, without warranty, 2026 by Verilator Authors.
// SPDX-FileCopyrightText: 2026 Wilson Snyder
// SPDX-License-Identifier: CC0-1.0

class collection;
  rand bit [2:0] data[$];
  constraint values {
    foreach (data[i]) data[i] dist {2 :/ 3, 5 :/ 1};
  }
endclass

module t;
  initial begin
    automatic collection c = new;
    c.data = '{2, 5};
    c.data.rand_mode(0);
    repeat (8) begin
      if (c.randomize() != 1) $stop;
      if (c.data.size() != 2 || c.data[0] != 2 || c.data[1] != 5) $stop;
    end
    c.data[1] = 0;
    if (c.randomize() != 0) $stop;
    c.data.rand_mode(1);
    repeat (8) begin
      if (c.randomize() != 1) $stop;
      if (c.data.size() != 2) $stop;
      foreach (c.data[i]) begin
        if (!(c.data[i] inside {2, 5})) $stop;
      end
    end
    $write("*-* All Finished *-*\n");
    $finish;
  end
endmodule
