// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain, for
// any use, without warranty, 2022 by Antmicro Ltd.
// SPDX-License-Identifier: CC0-1.0

module t;
   logic clk = 0;
   always #1 clk = ~clk;

   event eventD;
   logic sig1 = 0;
   logic sig2 = 1;
   int cyc = 0;
   always @(posedge clk) begin
      cyc <= cyc + 1;
      if (cyc % 5 == 0) ->eventD;
      else if (cyc % 5 == 1) sig1 = 1;
      else if (cyc % 5 == 2) sig2 = 0;
      else if (cyc % 5 == 3) sig1 = 0;
      else if (cyc % 5 == 4) sig1 = 1;
   end
   initial forever begin
      @(posedge sig1, eventD)
      $write("[%0t] POS sig1 or event D\n", $time);
      @(posedge sig2, eventD)
      $write("[%0t] POS sig2 or event D\n", $time);
      @(posedge sig1, negedge sig2)
      $write("[%0t] POS sig1 or NEG sig2\n", $time);
      @(negedge sig1, posedge sig2)
      $write("[%0t] NEG sig1 or POS sig2\n", $time);
      $write("*-* All Finished *-*\n");
      $finish;
   end
endmodule

