// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain, for
// any use, without warranty, 2024 by Antmicro.
// SPDX-License-Identifier: CC0-1.0

module t(
   clk
   );
   input clk;
   logic[15:0] large_out;
   logic small_out;

   Sub sub(.large_out(large_out), .small_out(small_out));

   always @ (posedge clk) begin
      if (sub.large_tmp !== 16'hfa15) begin
         $write("Mismatch: sub.large_tmp (%d) !== 16'hfa15\n", sub.large_tmp);
         $stop;
      end
      if (sub.small_tmp !== 1) begin
         $write("Mismatch: sub.small_tmp (%d) !== 1\n", sub.small_tmp);
         $stop;
      end
      if (large_out !== 16'hfa15) begin
         $write("Mismatch: large_out (%d) !== 16'hfa15\n", large_out);
         $stop;
      end
      if (small_out !== 1) begin
         $write("Mismatch: small_out (%d) !== 1\n", small_out);
         $stop;
      end
      $write("*-* All Finished *-*\n");
      $finish;
   end
endmodule

module Sub
   (output logic[15:0] large_out, output logic small_out); /*verilator hier_block*/
   logic[15:0] large_tmp;
   logic small_tmp;
   logic[15:0] inner_tmp;
   assign inner_tmp = 16'hfa15;
   assign large_tmp = inner_tmp;
   assign small_tmp = 1;
   assign large_out = large_tmp;
   assign small_out = small_tmp;
endmodule
