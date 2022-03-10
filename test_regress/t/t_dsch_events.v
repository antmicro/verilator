// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain, for
// any use, without warranty, 2022 by Krzysztof Bieganski.
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
   end
   initial for (int i = 0; i < N; i++) begin
      #1 $write("triggering events[%0d]\n", i);
      ->events[i];
   end

   event eventA;
   event eventB;
   event eventC;
   initial forever begin
      #2
      $write("triggering eventA\n");
      ->eventA;
      #2
      $write("triggering eventB\n");
      ->eventB;
      #2
      $write("triggering eventC\n");
      ->eventC;
   end
   initial for (int i = 0; i < N/2; i++) begin
      $write("waiting for eventA, eventB, or eventC\n");
      @(eventA, eventB, eventC)
      if (!eventA.triggered && !eventB.triggered && !eventC.triggered) $stop;
      $write("got either eventA, eventB, or eventC\n");
   end

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
   end

   initial #N begin
      $write("*-* All Finished *-*\n");
      $finish;
   end
endmodule
