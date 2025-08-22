// DESCRIPTION: Verilator: Verilog Test module for SystemVerilog 'alias'
//
// Alias type check error test.
//
// This file ONLY is placed under the Creative Commons Public Domain, for
// any use, without warranty, 2025 by Antmicro Ltd.
// SPDX-License-Identifier: CC0-1.0

module t (/*AUTOARG*/
   // Inputs
   clk
   );
   input clk;

   reg [31:0] out = 32'h0;

   sub sub_i (.clk (clk), .out (out));

   wire [31:0] a;

   alias a = sub_i.out;

endmodule

module sub (input clk, output wire [31:0] out);

   reg [31:0] temp = 32'h0;

   assign out = temp;

   always @ (posedge clk) begin

      temp += 1;

   end

endmodule