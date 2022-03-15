// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain, for
// any use, without warranty, 2022 by Antmicro Ltd.
// SPDX-License-Identifier: CC0-1.0

module t;
   int   cyc       = 0;
   logic clk1      = 0;
   logic clk2      = 0;
   logic clk1_copy;
   logic clk2_inv;

   initial forever #1 clk1 = ~clk1;

   always @(negedge clk1) $write("[%0t] NEG clk1 (%b)\n", $time, clk1);

   always @(posedge clk1) begin
      cyc <= cyc + 1;
      $write("[%0t] POS clk1 (%b)\n", $time, clk1);
      if (cyc == 5) begin
         $write("*-* All Finished *-*\n");
         $finish;
      end
   end

   always #2 clk2 = ~clk2;
   always @(negedge clk2) $write("[%0t] NEG clk2 (%b)\n", $time, clk2);
   always @(posedge clk2) $write("[%0t] POS clk2 (%b)\n", $time, clk2);

   assign clk1_copy = clk1;
   initial forever begin
       @(negedge clk1_copy) $write("[%0t] NEG clk1_copy (%b)\n", $time, clk1_copy);
       @(posedge clk1_copy) $write("[%0t] POS clk1_copy (%b)\n", $time, clk1_copy);
   end

   assign clk2_inv = ~clk2;
   initial forever begin
       @(negedge clk2_inv) $write("[%0t] NEG clk2_inv (%b)\n", $time, clk2_inv);
       @(posedge clk2_inv) $write("[%0t] POS clk2_inv (%b)\n", $time, clk2_inv);
   end

   always @(posedge clk1 or negedge clk2) $write("[%0t] POS clk1 (%b) or NEG clk2 (%b)\n", $time, clk1, clk2);
endmodule
