// DESCRIPTION: Verilator: Verilog Test module for SystemVerilog 'alias'
//
// Alias width check error test.
//
// This file ONLY is placed under the Creative Commons Public Domain, for
// any use, without warranty, 2013 by Jeremy Bennett.
// SPDX-License-Identifier: CC0-1.0

module t (/*AUTOARG*/
   // Inputs
   clk
   );
   input clk;

   bit a;
   logic b;

   alias a = b;

endmodule
