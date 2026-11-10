// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain.
// SPDX-FileCopyrightText: 2026 The Verilator Authors
// SPDX-License-Identifier: CC0-1.0

module t;
  typedef enum bit [1:0] {
    A,
    B,
    C,
    D
  } key_t;
  typedef enum bit [1:0] {OTHER = 2} other_t;
  int entries[key_t];
  int sum;
  bit [9:0] wide_key;

  initial begin
    // IEEE 1800-2023 7.8.4 casts integral subscripts to the declared key type.
    // verilator lint_off WIDTHTRUNC
    for (int i = 0; i < 4; ++i) entries[i] = 40 + i;
    wide_key = 10'd258;
    entries[wide_key]++;
    // verilator lint_on WIDTHTRUNC
    if (entries[C] != 43 || entries[OTHER] != 43 || entries.num() != 4) $stop;
    foreach (entries[key]) sum += entries[key];
    if (sum != 167) $stop;
    $write("*-* All Finished *-*\n");
    $finish;
  end
endmodule
