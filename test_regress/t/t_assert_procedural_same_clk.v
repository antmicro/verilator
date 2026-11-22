// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain.
// SPDX-FileCopyrightText: 2026 Wilson Snyder
// SPDX-License-Identifier: CC0-1.0

// verilog_format: off
`define stop $stop
`define checkd(gotv,expv) do if ((gotv) !== (expv)) begin $write("%%Error: %s:%0d:  got=%0d exp=%0d\n", `__FILE__,`__LINE__, (gotv), (expv)); `stop; end while(0);
// verilog_format: on

// A named property clocked by its enclosing always must sample once per edge,
// advancing $past and running each enabled pass or failure action once.
module t;
  bit clk;
  bit data;
  bit enabled;
  bit next_enable;
  int passes;
  int failures;

  always @(posedge clk) enabled <= next_enable;

  property stable_data;
    @(posedge clk) disable iff (!enabled) data == $past(
        data
    );
  endproperty
  always @(posedge clk) begin
    assert property (stable_data) passes++;
    else failures++;
  end
  task tick(bit next_data);
    data = next_data;
    #1 clk = 1;
    #1 clk = 0;
  endtask
  initial begin
    tick(0);
    next_enable = 1;
    tick(0);
    tick(1);
    tick(1);
    tick(0);
    tick(1);
    next_enable = 0;
    tick(0);
    `checkd(passes, 2);
    `checkd(failures, 3);
    $display("*-* All Finished *-*");
    $finish;
  end
endmodule
