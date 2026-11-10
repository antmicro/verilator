// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain.
// SPDX-FileCopyrightText: 2026 The Verilator Authors
// SPDX-License-Identifier: CC0-1.0

module t;
  typedef enum bit [1:0] {A, B, C} key_t;
  int keyed[key_t];
  int plain[int];
  real index_value = 1.5;
  initial begin
    keyed[index_value] = 1;
    plain[index_value] = 1;
  end
endmodule
