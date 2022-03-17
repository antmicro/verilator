// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain, for
// any use, without warranty, 2022 by Antmicro Ltd.
// SPDX-License-Identifier: CC0-1.0

module t;
   logic clk = 0;
   always #2 clk = ~clk;

   event ping;
   event pong;
   int cnt = 0;
   always @ping begin
      $write("ping ->\n");
      ->pong;
   end
   initial forever begin
      @pong $write("<- pong\n");
      cnt++;
      if (cnt == 5) begin
         $write("*-* All Finished *-*\n");
         $finish;
      end else ->ping;
   end
   initial ->ping;
endmodule
