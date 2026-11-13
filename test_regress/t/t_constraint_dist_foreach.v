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

  class DistForeachTest;
    rand bit bits[8];

    constraint c {
      foreach (bits[i]) {
        bits[i] dist {1'b0 := 1, 1'b1 := 1};
      }
    }
  endclass

  bit saw_mixed;
  bit all_zero;
  bit all_one;

  initial begin
    int randomize_result;
    static DistForeachTest test = new();
    saw_mixed = 0;

    repeat (30) begin
      all_zero = 1;
      all_one = 1;

      randomize_result = test.randomize();
      `checkd(randomize_result, 1)
      foreach (test.bits[i]) begin
        all_zero &= (test.bits[i] == 1'b0);
        all_one &= (test.bits[i] == 1'b1);
      end
      saw_mixed |= !(all_zero || all_one);
    end

    `checkd(saw_mixed, 1)
    $write("*-* All Finished *-*\n");
    $finish;
  end

endmodule
