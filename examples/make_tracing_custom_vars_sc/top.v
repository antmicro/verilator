// DESCRIPTION: Verilator: Verilog example module
//
// This file ONLY is placed under the Creative Commons Public Domain, for
// any use, without warranty, 2003 by Wilson Snyder.
// SPDX-License-Identifier: CC0-1.0
// ======================================================================

// This is intended to be a complex example of several features, please also
// see the simpler examples/make_hello_c.

module top (
    input wire clk
);
  logic [31:0] cyc;
  initial cyc = 0;

  always @(posedge clk) begin
    cyc <= cyc + 1;
    if (cyc == 5) begin
      $display("*-* All Finished *-*\n");
      $finish;
    end
  end
endmodule
