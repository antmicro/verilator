// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain, for
// any use, without warranty, 2024 by Antmicro Ltd.
// SPDX-License-Identifier: CC0-1.0

module t(
   clk
   );
   input clk;
   logic [31:0] out1;

   Test #(.PARAM(2)) test1(.out (out1));

   logic [31:0] out2;
   Test #(.PARAM(3)) test2(.out (out2));

   always @ (posedge clk) begin
      if (out1 !== 2) $stop;
      if (out2 !== 3) $stop;
      $write("*-* All Finished *-*\n");
      $finish;
   end

endmodule

module Test
   #(parameter PARAM = 0)
   (
   output logic[31:0] out
   ); /*verilator hier_block*/

   SubTest #(.PARAM(PARAM)) subTest(.out(out));
endmodule

module SubTest
   #(parameter PARAM = 1)
   (
   output logic[31:0] out
   ); /*verilator hier_block*/

   assign out = PARAM;
endmodule
