// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain.
// SPDX-FileCopyrightText: 2026 Wilson Snyder
// SPDX-License-Identifier: CC0-1.0

interface item_if;
  int value;
endinterface

module t;
  for (genvar i = 0; i < 2; ++i) begin : items
    item_if inst();
  end
  virtual item_if first, second;
  initial begin
    first = items[0].inst;
    second = items[1].inst;
    first.value = 11;
    second.value = 12;
    if (items[0].inst.value != 11 || items[1].inst.value != 12) $stop;
    $display("*-* All Finished *-*");
    $finish;
  end
endmodule
