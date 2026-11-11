// DESCRIPTION: Check inline element constraints after the solver resizes a typedef'd queue.
// SPDX-FileCopyrightText: 2026 The Verilator Authors
// SPDX-License-Identifier: CC0-1.0

typedef bit [6:0] queue_t[$];

class Sample;
  rand queue_t data;
  constraint extent {data.size() == 2;}
endclass

module t;
  initial begin
    Sample obj;
    obj = new;
    if (0 == obj.randomize() with {foreach (data[i]) data[i] == 7'(i + 17);}) $stop;
    if (obj.data.size() != 2 || obj.data[0] != 17 || obj.data[1] != 18) $stop;
    $display("*-* All Finished *-*");
    $finish;
  end
endmodule
