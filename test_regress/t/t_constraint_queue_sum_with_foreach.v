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

  class QueueSumWithForeachTest;
    rand bit [4:0] q[$];
    rand bit [2:0] pair_count;
    rand bit [4:0] pair_value[4];
    rand bit [1:0] pair_id[8];

    constraint c_size {
      q.size() == 4;
      pair_count == 2;
    }

    constraint c_pair_value {
      foreach (pair_value[idx]) {
        if (idx < pair_count) {
          pair_value[idx] inside {[5'd0 : 5'd31]};
        } else {
          pair_value[idx] == 5'd0;
        }
      }

      pair_value[0] != pair_value[1];
    }

    constraint c_pair_id {
      foreach (pair_id[idx]) {
        if (idx < q.size()) {
          int'(pair_id[idx]) < int'(pair_count);
        } else {
          pair_id[idx] == 2'd3;
        }
      }

      foreach (pair_value[idx]) {
        if (idx < pair_count) {
          pair_id.sum() with (int'(int'(item) == idx)) == 2;
        }
      }
    }

    constraint c_queue_value {
      foreach (q[idx]) {
        q[idx] == pair_value[pair_id[idx]];
      }
    }
  endclass

  initial begin
    int randomize_result;
    static QueueSumWithForeachTest test = new();

    repeat (10) begin
      int counts[4];
      for (int idx = 0; idx < 4; idx++) counts[idx] = 0;

      randomize_result = test.randomize();
      `checkd(randomize_result, 1)
      `checkd(test.q.size(), 4)
      foreach (test.q[idx]) begin
        counts[test.pair_id[idx]]++;
        `checkd(test.q[idx], test.pair_value[test.pair_id[idx]])
      end
      `checkd(counts[0], 2)
      `checkd(counts[1], 2)
    end

    $write("*-* All Finished *-*\n");
    $finish;
  end

endmodule
