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

   wire [31:0] a;
   int b;

   alias {a[7:0],a[15:8],a[23:16],a[31:24]} = {b[7:0],b[15:8],b[23:16],b[31:24]};

endmodule
