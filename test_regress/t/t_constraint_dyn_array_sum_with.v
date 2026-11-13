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

  class SumWithTest;
    rand bit [7:0] data[];
    rand int unsigned total;

    function new();
      data = new[4];
    endfunction

    constraint c_size {data.size() == 4;}
    constraint c_domain {
      foreach (data[i]) data[i] inside {[1 : 10]};
    }
    constraint c_sum {
      total == data.sum() with (int'(item));
      total == 20;
    }
  endclass

  class CountWithTest;
    rand bit [7:0] data[];
    rand bit [7:0] repeated_value;

    function new();
      data = new[5];
    endfunction

    constraint c_size {data.size() == 5;}
    constraint c_domain {
      repeated_value inside {[1 : 9]};
      foreach (data[i]) data[i] inside {[1 : 9]};
    }
    constraint c_count {
      data.sum() with (int'(item == repeated_value)) == 3;
    }
  endclass

  initial begin
    int randomize_result;
    static SumWithTest sum_test = new();
    static CountWithTest count_test = new();

    repeat (10) begin
      int unsigned sum_exp;
      int unsigned count_exp;

      randomize_result = sum_test.randomize();
      `checkd(randomize_result, 1)
      sum_exp = 0;
      foreach (sum_test.data[i]) sum_exp += int'(sum_test.data[i]);
      `checkd(sum_test.total, 20)
      `checkd(sum_exp, sum_test.total)

      randomize_result = count_test.randomize();
      `checkd(randomize_result, 1)
      count_exp = 0;
      foreach (count_test.data[i]) begin
        if (count_test.data[i] == count_test.repeated_value) count_exp++;
      end
      `checkd(count_exp, 3)
    end

    $write("*-* All Finished *-*\n");
    $finish;
  end

endmodule
