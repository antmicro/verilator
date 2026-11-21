// DESCRIPTION: Verilator: Awaited sensitivities sharing trigger words and edge state
//
// This file ONLY is placed under the Creative Commons Public Domain.
// SPDX-FileCopyrightText: 2026 The Verilator Authors
// SPDX-License-Identifier: CC0-1.0

module t(input logic [1024:0] clk, output logic [15:0] count [1025]);
  for (genvar i = 0; i < 1025; ++i) begin
    initial count[i] = 0;
    initial forever begin
      @(posedge clk[i]);
      count[i] <= count[i] + 1;
    end
  end
endmodule
