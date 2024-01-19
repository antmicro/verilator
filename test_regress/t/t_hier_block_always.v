// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain, for
// any use, without warranty, 2024 by Antmicro Ltd.
// SPDX-License-Identifier: CC0-1.0

module t(
   clk
   );
   input clk;
   reg tmp_in;
   wire tmp_out;

   assign tmp_in = 0;

   Test test(.tmp_out (tmp_out), .tmp_in (tmp_in));

   always @ (posedge clk) begin
      if (tmp_out !== ~tmp_in) $stop;
      $write("*-* All Finished *-*\n");
      $finish;
   end

endmodule

module Test(
   output wire tmp_out,
   input reg tmp_in
   ); /*verilator hier_block*/

   assign tmp_out = ~ tmp_in;
endmodule
