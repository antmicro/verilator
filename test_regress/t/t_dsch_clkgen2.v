// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain, for
// any use, without warranty, 2022 by Antmicro Ltd.
// SPDX-License-Identifier: CC0-1.0

module t;
   int   cyc = 0;
   logic clk = 0;
   logic clk_inv;

   always #2 clk = ~clk;
   always @(negedge clk) $write("[%0t] NEG clk (%b)\n", $time, clk);
   always @(posedge clk) $write("[%0t] POS clk (%b)\n", $time, clk);

   assign #1 clk_inv = ~clk;
   initial forever begin
       @(posedge clk_inv) $write("[%0t] POS clk_inv (%b)\n", $time, clk_inv);
       @(negedge clk_inv) $write("[%0t] NEG clk_inv (%b)\n", $time, clk_inv);
   end

   initial #20 begin
       $write("*-* All Finished *-*\n");
       $finish;
   end
endmodule
