// DESCRIPTION: Check unconstrained scope randomization picks only declared enum literals.
//
// This file ONLY is placed under the Creative Commons Public Domain.
// SPDX-FileCopyrightText: 2026 The Verilator Authors
// SPDX-License-Identifier: CC0-1.0

// verilog_format: off
`define stop $stop
`define checkh(gotv,expv) do if ((gotv) !== (expv)) begin $write("%%Error: %s:%0d:  got=%0x exp=%0x (%s !== %s)\n", `__FILE__,`__LINE__, (gotv), (expv), `"gotv`", `"expv`"); `stop; end while(0);
// verilog_format: on


module t;
  typedef enum int {
    Low = -3,
    Mid = 9,
    High = 17
  } level_t;
  typedef enum bit [95:0] {
    First = 1,
    Last = 96'h8_00000000_00000002
  } wide_t;
  level_t level;
  level_t fixed_values[2], dynamic_values[], queue_values[$], assoc_values[int];
  wide_t wide;

  initial begin
    int success;
    level_t first;
    bit changed;
    dynamic_values = new[2];
    queue_values.push_back(level_t'(0));
    assoc_values[-4] = level_t'(0);
    for (int i = 0; i < 32; ++i) begin
      success = std::randomize(level);
      `checkh(success, 1);
      `checkh((level inside {Low, Mid, High}), 1'b1);
      if (i == 0) first = level;
      else changed |= level != first;

      // Apply the element type at each container leaf, including when multiple
      // arguments of different types are randomized by the same call.
      success = std::randomize(fixed_values, dynamic_values, queue_values, assoc_values, wide);
      `checkh(success, 1);
      `checkh(dynamic_values.size(), 2);
      `checkh(queue_values.size(), 1);
      `checkh(assoc_values.exists(-4), 1);
      foreach (fixed_values[j]) `checkh((fixed_values[j] inside {Low, Mid, High}), 1'b1);
      foreach (dynamic_values[j]) `checkh((dynamic_values[j] inside {Low, Mid, High}), 1'b1);
      `checkh((queue_values[0] inside {Low, Mid, High}), 1'b1);
      `checkh((assoc_values[-4] inside {Low, Mid, High}), 1'b1);
      `checkh((wide inside {First, Last}), 1'b1);
    end
    `checkh(changed, 1'b1);
    $display("*-* All Finished *-*");
    $finish;
  end
endmodule
