// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain, for
// any use, without warranty, 2023 by Antmicro Ltd.
// SPDX-License-Identifier: CC0-1.0

`define N 10

class cls;
   task reset;
      $urandom(0);
   endtask
   task await_all(process p[]);
      foreach (p[i]) wait (p[i]);
      foreach (p[i]) p[i].await();
   endtask
endclass

module t;
   initial begin
`ifdef INIT_PROCESS
      // Make this initial a controllable process too
      process root = process::self();
`endif
      cls c = new;
      process p[] = new[`N];
      integer sumA = 0;
      integer sumB = 0;

      c.reset();
      foreach (p[i]) fork
         begin
            p[i] = process::self();
            sumA = sumA + $random;
         end
      join_none
      c.await_all(p);

      c.reset();
      foreach (p[i]) fork
         begin
            p[i] = process::self();
            sumB = sumB + $random;
         end
      join_none
      c.await_all(p);

      $display("%d %d", sumA, sumB);
      if (sumA != sumB) $stop;
      $write("*-* All Finished *-*\n");
      $finish;
   end

   initial begin
      // c.reset();
   end
endmodule
