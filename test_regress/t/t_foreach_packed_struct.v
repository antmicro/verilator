// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain.
// SPDX-FileCopyrightText: 2026 Wilson Snyder
// SPDX-License-Identifier: CC0-1.0

module t;
  typedef struct packed {
    logic flag;
    logic [2:0] count;
  } value_t;

  function automatic int collect(value_t value);
    int index_sum = 0;
    foreach (value[i]) index_sum = 10 * index_sum + i;
    return index_sum;
  endfunction

  initial begin
    if (collect('0) != 3210) $fatal(1, "indices were %0d", collect('0));
    $display("*-* All Finished *-*");
    $finish;
  end
endmodule
