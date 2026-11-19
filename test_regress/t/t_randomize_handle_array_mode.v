// DESCRIPTION: Verilator: Randomization of class handles in arrays
//
// This file ONLY is placed under the Creative Commons Public Domain.
// SPDX-FileCopyrightText: 2026 The Verilator Authors
// SPDX-License-Identifier: CC0-1.0

// verilog_format: off
`define stop $stop
`define checkd(gotv,expv) do if ((gotv) !== (expv)) begin $write("%%Error: %s:%0d:  got=%0d exp=%0d\n", `__FILE__,`__LINE__, (gotv), (expv)); `stop; end while(0);
// verilog_format: on

class Leaf;
  rand bit [7:0] data;
  int calls;

  function void post_randomize();
    ++calls;
  endfunction
endclass

class Holder;
  rand Leaf items[string];
  rand Leaf fixed_items[-3:0];
  Leaf dynamic_items[int][];
  Leaf queue_items[string][$];
endclass

module t;
  initial begin
    Holder obj;
    Leaf leaf;
    int success;
    obj = new;
    leaf = new;
    leaf.data = 13;
    obj.items["key"] = leaf;
    foreach (obj.fixed_items[i]) obj.fixed_items[i] = new;

    // Disabling the array protects its child's value and suppresses its callback.
    obj.items.rand_mode(0);
    success = obj.randomize();
    `checkd(success, 1);
    `checkd(leaf.calls, 0);
    `checkd(leaf.data, 8'd13);

    // A signed declared index must visit each normalized storage offset exactly once.
    foreach (obj.fixed_items[i]) begin
      `checkd(obj.fixed_items[i].calls, 1);
    end

    obj.items.rand_mode(1);
    success = obj.randomize();
    `checkd(success, 1);
    `checkd(leaf.calls, 1);

    // Randomizing an array element changes its object, not the stored handle.
    for (int i = -1; i < 2; ++i) begin
      obj.dynamic_items[i] = new[3];
      for (int j = 0; j < 3; ++j) obj.dynamic_items[i][j] = new;
    end
    foreach (obj.dynamic_items[i, j]) begin
      obj.dynamic_items[i][j].srandom(i * 3 + j);
      `checkd(obj.dynamic_items[i][j].randomize(), 1);
      `checkd(obj.dynamic_items[i][j].calls, 1);
    end
    `checkd(obj.dynamic_items[99][0].randomize(), 0);
    `checkd(obj.dynamic_items.exists(99), 0);
    `checkd(obj.dynamic_items[0][3].randomize(), 0);
    `checkd(obj.dynamic_items[0].size(), 3);

    for (int j = 0; j < 3; ++j) begin
      automatic Leaf entry = new;
      obj.queue_items["key"].push_back(entry);
    end
    foreach (obj.queue_items[key, j]) begin
      `checkd(obj.queue_items[key][j].randomize(), 1);
      `checkd(obj.queue_items[key][j].calls, 1);
    end
    obj.queue_items["key"][$].srandom(42);
    `checkd(obj.queue_items["key"][$].randomize(), 1);
    `checkd(obj.queue_items["key"][$].calls, 2);
    `checkd(obj.queue_items["missing"][$].randomize(), 0);
    `checkd(obj.queue_items.exists("missing"), 0);
    `checkd(obj.queue_items["key"].size(), 3);

    $write("*-* All Finished *-*\n");
    $finish;
  end
endmodule
