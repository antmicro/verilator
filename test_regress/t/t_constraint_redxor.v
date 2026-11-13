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

  class ParityRecordTest;
    rand bit [11:0] data_bits;
    rand bit parity_bit;
    rand bit [11:0] select_mask;
    rand bit [11:0] flipped_bits;
    rand bit selected_was_one;

    constraint bitvec_c {
      $countones(data_bits) inside {[5:8]};
      parity_bit == ~(^data_bits);
      $onehot(select_mask);
      flipped_bits == (data_bits ^ select_mask);
      selected_was_one == ((data_bits & select_mask) != 0);
      (^flipped_bits) == parity_bit;
    }
  endclass

  class PayloadTest;
    rand bit [7:0] payload_mask;
    rand bit [7:0] select_mask;
    rand bit [7:0] armed_mask;
    rand bit selected_present;
    rand bit odd_armed;

    constraint payload_select_c {
      $countones(payload_mask) == 4;
      $countones(payload_mask[7:4]) == 2;
      $countones(payload_mask[3:0]) == 2;
      $onehot(select_mask);
      armed_mask == (payload_mask | select_mask);
      selected_present == |(payload_mask & select_mask);
      odd_armed == ^armed_mask;
    }
  endclass

  initial begin
    int randomize_result;
    static ParityRecordTest parity_test = new();
    static PayloadTest payload_test = new();

    repeat (20) begin
      randomize_result = parity_test.randomize();
      `checkd(randomize_result, 1)
      `checkd(($countones(parity_test.data_bits) >= 5), 1)
      `checkd(($countones(parity_test.data_bits) <= 8), 1)
      `checkd(parity_test.parity_bit, ~(^parity_test.data_bits))
      `checkd($onehot(parity_test.select_mask), 1)
      `checkd(parity_test.flipped_bits,
              parity_test.data_bits ^ parity_test.select_mask)
      `checkd(parity_test.selected_was_one,
              ((parity_test.data_bits & parity_test.select_mask) != 0))
      `checkd((^parity_test.flipped_bits), parity_test.parity_bit)

      randomize_result = payload_test.randomize();
      `checkd(randomize_result, 1)
      `checkd($countones(payload_test.payload_mask), 4)
      `checkd($countones(payload_test.payload_mask[7:4]), 2)
      `checkd($countones(payload_test.payload_mask[3:0]), 2)
      `checkd($onehot(payload_test.select_mask), 1)
      `checkd(payload_test.armed_mask,
              payload_test.payload_mask | payload_test.select_mask)
      `checkd(payload_test.selected_present,
              |(payload_test.payload_mask & payload_test.select_mask))
      `checkd(payload_test.odd_armed, ^payload_test.armed_mask)
    end

    $write("*-* All Finished *-*\n");
    $finish;
  end

endmodule
