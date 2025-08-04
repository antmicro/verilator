// DESCRIPTION: Verilator: Verilog example module
//
// This file ONLY is placed under the Creative Commons Public Domain, for
// any use, without warranty, 2025 by Antmicro.
// SPDX-License-Identifier: CC0-1.0
// ======================================================================

module sub(input clk);/*verilator hier_block*/
  // Signal to trace driven from Verilog
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
