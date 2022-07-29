// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain, for
// any use, without warranty, 2022 by Antmicro Ltd.
// SPDX-License-Identifier: CC0-1.0

module t (/*AUTOARG*/
   // Inputs
   clk
   );
   input clk;

   reg [3:0] a, b;

   initial begin
      a = 0;
      b = 0;
   end

   always @ (posedge clk) begin
      a <= a + 1;
      b = b + 1;

      $strobe("%0d == %0d, %0d == %0d", a, b, $past(a), $past(b));

      if (b >= 10) begin
         $write("*-* All Finished *-*\n");
         $finish;
      end
   end
endmodule
