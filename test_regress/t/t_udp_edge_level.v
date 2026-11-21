// DESCRIPTION: Check edge and level rows of a sequential UDP
//
// This file ONLY is placed under the Creative Commons Public Domain.
// SPDX-FileCopyrightText: 2026 The Verilator Authors
// SPDX-License-Identifier: CC0-1.0

// verilog_format: off
`define stop $stop
`define checkd(gotv,expv) do if ((gotv) !== (expv)) begin $write("%%Error: %s:%0d:  got=%0d exp=%0d (%s !== %s)\n", `__FILE__,`__LINE__, (gotv), (expv), `"gotv`", `"expv`"); `stop; end while(0);
// verilog_format: on

primitive dff(q, clk, d, clear_n, set_n);
  output q;
  reg q;
  input clk, d, clear_n, set_n;
  table
    r 0 1 1 : ? : 0;
    r 1 1 1 : ? : 1;
    ? ? 0 ? : ? : 0;
    ? ? 1 0 : ? : 1;
    f ? 1 1 : ? : -;
    ? * 1 1 : ? : -;
    ? ? r 1 : ? : -;
    ? ? 1 r : ? : -;
  endtable
endprimitive

module t;
  bit clk, d, set_n;
  bit clear_n = 1;
  wire q;
  dff dut(q, clk, d, clear_n, set_n);

  initial begin
    #1;
    `checkd(q, 1);
    clear_n = 0;
    #1;
    `checkd(q, 0);
    set_n = 1;
    clear_n = 1;
    d = 1;
    #1;
    `checkd(q, 0);
    clk = 1;
    #1;
    `checkd(q, 1);
    d = 0;
    clk = 0;
    #1;
    `checkd(q, 1);
    clk = 1;
    #1;
    `checkd(q, 0);
    set_n = 0;
    #1;
    `checkd(q, 1);
    $display("*-* All Finished *-*");
    $finish;
  end
endmodule
