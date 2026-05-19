// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain.
// SPDX-FileCopyrightText: 2026 Antmicro
// SPDX-License-Identifier: CC0-1.0

`define stop $stop
`define checkh(gotv,
               expv) do if ((gotv) !== (expv)) begin $write("%%Error: %s:%0d:  got='h%p exp='h%p\n", `__FILE__,`__LINE__, (gotv), (expv)); `stop; end while(0)

module t (  /*AUTOARG*/
    // Inputs
    clk
);

  input clk;

  typedef struct {
    int fails;
    int passs;
  } result_t;

  result_t results[int];
  result_t expected[int];

  localparam MAX = 15;
  integer cyc = 1;

  assert property (@(posedge clk) eventually [1:2] 1'b0)
    results[1].passs++;
  else results[1].fails++;

  always @(edge clk) begin
    ++cyc;
    if (cyc == MAX) begin
      expected[1] = '{5, 0};
      `checkh(results, expected);
      $write("*-* All Finished *-*\n");
      $finish;
    end
  end
endmodule
