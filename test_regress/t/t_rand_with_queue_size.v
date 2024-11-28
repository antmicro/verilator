// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain, for
// any use, without warranty, 2024 by Antmicro Ltd.
// SPDX-License-Identifier: CC0-1.0

class Foo;
   rand int q[$];
   rand int q2[$][$];
   rand int r;
   int      x = 1;
   constraint c {
      q.size() == 15;
      q2.size() == 10;
   }
endclass

module t;
   initial begin
      Foo foo = new;

      int res = foo.randomize() with {r < 5;};
      if (res != 1) $stop;
      if (foo.q.size() != 15) $stop;
      if (foo.q2.size() != 10) $stop;
      if (foo.r >= 5) $stop;

      $write("*-* All Finished *-*\n");
      $finish;
   end
endmodule
