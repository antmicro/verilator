// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain, for
// any use, without warranty, 2022 by Krzysztof Bieganski.
// SPDX-License-Identifier: CC0-1.0

module t;
   event e1;
   event e2;
   event e3;
   int counter = 0;

   // As Verilator doesn't support recursive calls, let's use macros to generate tasks
   `define FORK2_END(i) \
   task fork2_``i; \
      #1 counter++; \
   endtask

   `define FORK2(i, j) \
   task fork2_``i; \
      fork \
         begin \
            #1 fork2_``j; \
         end \
         begin \
            #1 fork2_``j; \
         end \
      join \
   endtask

   `FORK2_END(0);
   `FORK2(1, 0);
   `FORK2(2, 1);
   `FORK2(3, 2);
   `FORK2(4, 3);
   `FORK2(5, 4);
   `FORK2(6, 5);
   `FORK2(7, 6);
   `FORK2(8, 7);

   initial begin
      fork
         #8 $write("[%0t] forked process 1\n", $time);
         #4 $write("[%0t] forked process 2\n", $time);
         #2 $write("[%0t] forked process 3\n", $time);
         $write("[%0t] forked process 4\n", $time);
      join
      #8 $write("[%0t] main process\n", $time);
      fork
         $write("[%0t] forked process 1\n", $time);
         begin
            @e1;
            $write("[%0t] forked process 2\n", $time);
            ->e1;
         end
      join_any
      $write("[%0t] back in main process\n", $time);
      ->e1;
      @e1;
      fork // p1->e2  ==>  p2->e3  ==>  p3->e3  ==>  p2->e2  ==>  p1->e3  ==>  p3->e1
         begin
            #1 $write("[%0t] forked process 1\n", $time);
            ->e2;
            @e2 $write("[%0t] forked process 1 again\n", $time);
            ->e3;
         end
         begin
            @e2 $write("[%0t] forked process 2\n", $time);
            ->e3;
            @e3 $write("[%0t] forked process 2 again\n", $time);
            ->e2;
         end
         begin
            @e3 $write("[%0t] forked process 3\n", $time);
            ->e3;
            @e3 $write("[%0t] forked process 3 again\n", $time);
            ->e1;
         end
      join_none
      $write("[%0t] back in main process\n", $time);
      @e1;
      $write("[%0t] spawning 2^8 processes...\n", $time);
      fork2_8;
      $write("[%0t] process counter == %0d\n", $time, counter);
      $write("*-* All Finished *-*\n");
      $finish;
    end
endmodule
