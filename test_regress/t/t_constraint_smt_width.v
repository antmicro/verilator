// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain.
// SPDX-FileCopyrightText: 2026 Wilson Snyder
// SPDX-License-Identifier: CC0-1.0

// verilog_format: off
`define stop $stop
`define checkd(gotv,expv) do if ((gotv) !== (expv)) begin $write("%%Error: %s:%0d:  got=%0d exp=%0d\n", `__FILE__,`__LINE__, (gotv), (expv)); `stop; end while(0);
// verilog_format: on

module t;

  class DynamicIndexTest;
    rand bit [3:0] count;
    rand bit [2:0] points[];

    constraint c {
      count == 4'd6;
      points.size() == int'(count);
      points[0] == 3'd0;
      points[count - 1] == 3'd7;
      foreach (points[i]) points[i] inside {[3'd0:3'd7]};
    }
  endclass

  initial begin
    int randomize_result;
    static DynamicIndexTest test = new();

    repeat (20) begin
      randomize_result = test.randomize();
      `checkd(randomize_result, 1)
      `checkd(test.count, 6)
      `checkd(test.points.size(), int'(test.count))
      `checkd(test.points[0], 0)
      `checkd(test.points[test.count - 1], 7)
    end

    $write("*-* All Finished *-*\n");
    $finish;
  end

endmodule
