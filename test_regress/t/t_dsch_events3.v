// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain, for
// any use, without warranty, 2022 by Antmicro Ltd.
// SPDX-License-Identifier: CC0-1.0

module t;
   localparam N = 100;

   logic clk = 0;
   always #1 clk = ~clk;

   event events[N-1:0];
   initial begin
      for (int i = 0; i < N; i++) begin
         $write("waiting for events[%0d]\n", i);
         @events[i];
         if (!events[i].triggered) $stop;
      end
      $write("*-* All Finished *-*\n");
      $finish;
   end
   initial for (int i = 0; i < N; i++) begin
      #1 $write("triggering events[%0d]\n", i);
      ->events[i];
   end
endmodule
