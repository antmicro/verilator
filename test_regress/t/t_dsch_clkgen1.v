// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain, for
// any use, without warranty, 2022 by Antmicro Ltd.
// SPDX-License-Identifier: CC0-1.0

module t;
   int   cyc = 0;
   logic clk = 0;
   logic clk_copy;

   initial forever #1 clk = ~clk;

   always @(negedge clk) $write("[%0t] NEG clk (%b)\n", $time, clk);

   always @(posedge clk) begin
      cyc <= cyc + 1;
      $write("[%0t] POS clk (%b)\n", $time, clk);
      if (cyc == 5) begin
         $write("*-* All Finished *-*\n");
         $finish;
      end
   end

   assign clk_copy = clk;
   always @(posedge clk_copy or negedge clk_copy) $write("[%0t] POS/NEG clk_copy (%b)\n", $time, clk_copy);
endmodule

