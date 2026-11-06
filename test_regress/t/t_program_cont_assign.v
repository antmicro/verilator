// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain.
// SPDX-FileCopyrightText: 2026 Antmicro
// SPDX-License-Identifier: CC0-1.0

// verilog_format: off
`define stop $stop
`define checkd(gotv,expv) do if ((gotv) !== (expv)) begin $write("%%Error: %s:%0d:  got=%0d exp=%0d\n", `__FILE__,`__LINE__, (gotv), (expv)); `stop; end while(0);
// verilog_format: on

module t;
  bit clk = 0;
  wire reactive_clk;
  wire reactive_init;
  int value = 0;
  int checks = 0;
  int init_checks = 0;

  program_cont_assign test (
      .clk(clk),
      .reactive_clk(reactive_clk),
      .reactive_init(reactive_init)
  );

  always @(posedge clk) value <= value + 1;

  always @(posedge reactive_clk) begin
    checks = checks + 1;
    `checkd(value, checks);
  end

  always @(posedge reactive_init) begin
    init_checks = init_checks + 1;
  end

  initial begin
    if (reactive_init === 1) $stop;
    repeat (8) #1 clk ^= 1;
    `checkd(checks, 4);
    `checkd(init_checks, 1);
    $write("*-* All Finished *-*\n");
    $finish;
  end
endmodule

program automatic program_cont_assign (
    input wire clk,
    output wire reactive_clk,
    output wire reactive_init
);
  assign reactive_clk = clk;
  assign reactive_init = 1'b1;
endprogram
