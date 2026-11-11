// DESCRIPTION: Verilator: Skip randomizing the elements of a disabled class handle array
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

    $write("*-* All Finished *-*\n");
    $finish;
  end
endmodule
