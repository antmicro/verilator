// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain, for
// any use, without warranty, 2014 by Wilson Snyder.
// SPDX-License-Identifier: CC0-1.0

parameter INSTANCES = 20;

module t ();
  logic clk = 0;
  always #5 clk = ~clk;

  generate
    for (genvar i = 0; i < INSTANCES; ++i) sub sub (.*);
  endgenerate

  initial begin
    $display("clk started with: %d", clk);
  end

always @(negedge clk) begin
    $finish;
  end


  always @(posedge clk) begin
    $display("posedge of clk on time: %0t", $time);
  end
endmodule

module sub(input clk);
  bit x;
  assign x = clk == 0 ? 1 : 0;
  always @(x) begin
    $display("%d", x);
  end
  
  subsub subsub_0(.y(x));
  subsub subsub_1(.y(x));
  subsub subsub_2(.y(x));
  subsub subsub_3(.y(x));
  subsub subsub_4(.y(x));
  subsub subsub_5(.y(x));
  subsub subsub_6(.y(x));
  subsub subsub_7(.y(x));
  subsub subsub_8(.y(x));
  subsub subsub_9(.y(x));
  subsub subsub_10(.y(x));
  subsub subsub_11(.y(x));
  subsub subsub_12(.y(x));
  subsub subsub_13(.y(x));
  subsub subsub_14(.y(x));
  subsub subsub_15(.y(x));
  subsub subsub_16(.y(x));
  subsub subsub_17(.y(x));
  subsub subsub_18(.y(x));
endmodule

module subsub(input y);
  bit z;
  assign z = y;
endmodule
