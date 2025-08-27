// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain, for
// any use, without warranty, 2025 by Antmicro.
// SPDX-License-Identifier: CC0-1.0

module t (  /*AUTOARG*/
    // Inputs
    clk
);
  input clk;

  wire [15:0] a, b, c;

  integer cyc = 0;

  alias a = b = c;

  assign a = 16'habcd;
  initial begin
    if (a != 16'habcd) $stop;
    if (b != 16'habcd) $stop;
    if (c != 16'habcd) $stop;
    $write("*-* All Finished *-*\n");
    $finish;

  end
endmodule
