// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain, for
// any use, without warranty, 2014 by Wilson Snyder.
// SPDX-License-Identifier: CC0-1.0

parameter INSTANCES = 10;
parameter VARS = 10;

module t (
    clk
);
  input clk;
  generate
    for (genvar i = 0; i < INSTANCES; ++i) sub sub ();
  endgenerate

  always @(posedge clk) begin
    $write("*-* All Finished *-*\n");
    $finish;
  end
endmodule

module sub;
  generate
    for (genvar i = 0; i < VARS; ++i) begin
      bit x;
      assign x = (i % 2 == 0);
    end
  endgenerate
endmodule
