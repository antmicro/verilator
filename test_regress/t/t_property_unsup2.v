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

  localparam MAX = 15;
  integer cyc = 1;

  assert property (@(posedge clk) cyc == 1 implies eventually[1: 2] cyc == 3);

  assert property (@(posedge clk) (eventually[0: 1] clk) implies (eventually[0: 1] clk));

  assert property (@(posedge clk) (eventually[0: 1] clk) implies (eventually[0: 1] clk))
    $display("PASSED");
  else $display("FAILED");

  assert property (@(posedge clk) (eventually[0: 1] clk) iff (eventually[0: 1] clk));

  assert property (@(posedge clk) (eventually[0: 1] clk) iff (eventually[0: 1] clk))
    $display("PASSED");
  else $display("FAILED");

  always @(edge clk) begin
    ++cyc;
    if (cyc == MAX) begin
      $write("*-* All Finished *-*\n");
      $finish;
    end
  end
endmodule
