// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain, for
// any use, without warranty, 2022 by Wilson Snyder.
// SPDX-License-Identifier: CC0-1.0

`define checkd(gotv,expv) do if ((gotv) !== (expv)) begin $write("%%Error: %s:%0d:  got=%0d exp=%0d\n", `__FILE__,`__LINE__, (gotv), (expv)); $stop; end while(0);

class Multiplier;
   function int multBy2(int x);
      return 2 * x;
  endfunction
endclass

module t(/*AUTOARG*/);

   int a;
   int b;
   int i;
   Multiplier mult;

   initial begin
      mult = new;
      a = 10;
      i = (a = 2);
      `checkd(a, 2); `checkd(i, 2);

      a = 10;
      i = (a += 2);
      `checkd(a, 12); `checkd(i, 12);

      a = 10;
      i = (a -= 2);
      `checkd(a, 8); `checkd(i, 8);

      a = 10;
      i = (a *= 2);
      `checkd(a, 20); `checkd(i, 20);

      a = 10;
      i = (a /= 2);
      `checkd(a, 5); `checkd(i, 5);

      a = 11;
      i = (a %= 2);
      `checkd(a, 1); `checkd(i, 1);

      a = 10;
      i = (a &= 2);
      `checkd(a, 2); `checkd(i, 2);

      a = 8;
      i = (a |= 2);
      `checkd(a, 10); `checkd(i, 10);

      a = 10;
      i = (a ^= 2);
      `checkd(a, 8); `checkd(i, 8);

      a = 10;
      i = (a <<= 2);
      `checkd(a, 40); `checkd(i, 40);

      a = 10;
      i = (a >>= 2);
      `checkd(a, 2); `checkd(i, 2);

      a = 10;
      i = (a >>>= 2);
      `checkd(a, 2); `checkd(i, 2);

      a = 10;
      i = (a = (b = 5));
      `checkd(a, 5); `checkd(i, 5); `checkd(b, 5);

      a = 10;
      b = 6;
      i = ((a += (b += 1) + 1));
      `checkd(a, 18); `checkd(i, 18); `checkd(b, 7);

      a = 1;
      i = mult.multBy2(a) + (a = 5);
      `checkd(a, 5); `checkd(i, 7);

      a = 1;
      i = (a = 5) + mult.multBy2(a);
      `checkd(a, 5); `checkd(i, 15);

      a = 1;
      i = (a = 5) + mult.multBy2(a) + (a = 3);
      `checkd(a, 3); `checkd(i, 18);

      a = 1;
      i = (a = 5) + mult.multBy2(a) + (a = 3) + mult.multBy2(a);
      `checkd(a, 3); `checkd(i, 24);

      $write("*-* All Finished *-*\n");
      $finish;
   end

endmodule
