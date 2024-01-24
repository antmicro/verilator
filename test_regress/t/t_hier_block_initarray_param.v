// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain, for
// any use, without warranty, 2024 by Antmicro Ltd.
// SPDX-License-Identifier: CC0-1.0

typedef logic [3:0]array_t[7:0];

module t(
   clk
   );
   input clk;
   array_t out1;
   array_t out2;
   array_t out3;

   parameter array_t array = '{4'b000, 4'b001, 4'b010, 4'b011, 4'b100, 4'b101, 4'b110, 4'b111};

   Test #(.ARRAY(array)) test(.out (out1));
   TestWithParam #(.P(5), .ARRAY(array)) testWithParam(.out (out2));
   TestWithTypeParam #(.ARRAY_T(array_t), .ARRAY(array)) testWithTypeParam(.out (out3));

   always @ (posedge clk) begin
      if (out1 != array) $stop;
      if (out2 != array) $stop;
      if (out3 != array) $stop;
      $write("*-* All Finished *-*\n");
      $finish;
   end

endmodule

module Test
   #(parameter array_t ARRAY [7:0] = '{8, 7, 6, 5, 4, 3, 2, 1})
   (
   output array_t out
   ); /*verilator hier_block*/

   assign out = ARRAY;
endmodule

module TestWithParam
   #(parameter P = 1, parameter array_t ARRAY [7:0] = '{8, 7, 6, 5, 4, 3, 2, 1})
   (
   output array_t out
   ); /*verilator hier_block*/

   assign out = ARRAY;
endmodule

module TestWithTypeParam
   #(parameter type ARRAY_T = logic[7:0], parameter ARRAY_T ARRAY [7:0] = '{8, 7, 6, 5, 4, 3, 2, 1})
   (
   output ARRAY_T out
   ); /*verilator hier_block*/

   assign out = ARRAY;
endmodule
