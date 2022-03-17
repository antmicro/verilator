// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain, for
// any use, without warranty, 2022 by Antmicro Ltd.
// SPDX-License-Identifier: CC0-1.0

module t;
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
   initial begin
      for (int i = 0; i < 10; i++) begin
          $write("waiting for eventA, eventB, or eventC\n");
          @(eventA, eventB, eventC)
          if (!eventA.triggered && !eventB.triggered && !eventC.triggered) $stop;
          $write("got either eventA, eventB, or eventC\n");
      end
      $write("*-* All Finished *-*\n");
      $finish;
   end
endmodule

