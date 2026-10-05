// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain.
// SPDX-FileCopyrightText: 2026 Wilson Snyder
// SPDX-License-Identifier: CC0-1.0

module t #(
    parameter int N = 4000
) (
    input logic [N-1:0] a,
    output logic [N-1:0] y
);
  generate
    for (genvar i = 0; i < N; ++i) begin
      logic local_signal;
      always_comb local_signal = a[i];
      assign y[i] = local_signal;
    end
  endgenerate
endmodule
