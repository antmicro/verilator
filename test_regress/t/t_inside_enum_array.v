// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain.
// SPDX-FileCopyrightText: 2026 The Verilator Authors
// SPDX-License-Identifier: CC0-1.0

module t;
  // verilator lint_off WIDTHTRUNC
  typedef enum bit [2:0] {
    A = 1,
    B = 3
  } e_t;
  e_t q[$] = {A, B};
  e_t a[2] = '{A, B};
  bit [7:0] v;
  typedef enum bit signed [2:0] {
    NEG = -1,
    POS = 2
  } signed_e;
  signed_e signed_q[$] = {NEG, POS};
  bit signed [7:0] signed_v;
  int calls;
  function automatic bit [7:0] sample ();
    calls++;
    return v;
  endfunction
  initial begin
    v = 3;
    if (!(sample () inside {q}) || !(v inside {a}) || calls != 1) $stop;
    v = 11;
    if (sample () inside {q} || v inside {a} || calls != 2) $stop;
    signed_v = -1;
    if (!(signed_v inside {signed_q})) $stop;
    signed_v = 7;
    if (signed_v inside {signed_q}) $stop;
    $write("*-* All Finished *-*\n");
    $finish;
  end
endmodule
