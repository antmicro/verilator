// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain, for
// any use, without warranty, 2022 by Krzysztof Bieganski.
// SPDX-License-Identifier: CC0-1.0

module t;
   logic clk = 0;
   always #2 clk = ~clk;

   event ping;
   event pong;
   int cnt = 0;
   always @ping begin
      $write("[%0t] ping ->\n", $time);
      ->pong;
   end
   initial forever
      if (cnt < 5) begin
         @pong $write("[%0t] <- pong\n", $time);
         cnt++;
         ->ping;
      end else break;
   initial ->ping;

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
   end
   
   logic flag_a;
   logic flag_b;
   always @(posedge clk)
   begin
      $display("[%0t] b <= 0", $time);
      flag_b <= 1'b0;
      #1;
      $display("[%0t] a <= 1", $time);
      flag_a <= 1'b1;
      #2;
      $display("[%0t] b <= 1", $time);
      flag_b <= 1'b1;
   end
   always @(flag_a)
   begin
      $display("[%0t] Checking if b == 0", $time);
      if (flag_b !== 1'b0) $stop;
      #3;
      $display("[%0t] Checking if b == 1", $time);
      $display("[%0t] b == %b", $time, flag_b);
      if (flag_b !== 1'b1) $stop;
      #6.5;
      $write("*-* All Finished *-*\n");
      $finish;
   end
endmodule

