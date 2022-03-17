// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain, for
// any use, without warranty, 2022 by Antmicro Ltd.
// SPDX-License-Identifier: CC0-1.0

module t;
   logic clk = 0;
   always #2 clk = ~clk;

   event e1;
   event e2;
   logic[1:0] triggered;
   always @(e1) begin
      $write("[%0t] e1\n", $time);
      triggered[0] = 1;
   end

   always @(e2) begin
      $write("[%0t] e2\n", $time);
      triggered[1] = 1;
   end

   int cyc;
   always @(posedge clk, negedge clk) begin
      $write("[%0t] cyc=%0d triggered=%5b\n", $time, cyc, triggered);
      cyc <= cyc + 1;
      if (cyc == 1) begin if (triggered != 0) $stop; end
      else if (cyc == 2) ->e1;
      else if (cyc == 3) ->e2;
      else if (cyc == 4) if (triggered != 2'b11) $stop;
      else begin
         $write("*-* All Finished *-*\n");
         $finish;
      end
   end
endmodule

