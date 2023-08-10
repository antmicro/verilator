// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain, for
// any use, without warranty, 2023 by Antmicro Ltd.
// SPDX-License-Identifier: CC0-1.0

module t;
   integer a = 0;
   integer b = 0;
   integer stage = 0;

   // Process A
   initial begin
      $urandom(0);
      stage = 1; // seed initiated
      wait (stage == 2); // wait for B to init seed
      a = $random;
      stage = 3; // radom used
   end

   // Process B
   initial begin
      wait (stage == 1); // wait for A to init seed
      $urandom(0);
      stage = 2; // seed initiated
      b = $random;
      wait (stage == 3); // wait for A to use random
      if (a != b) $stop;
      $write("*-* All Finished *-*\n");
      $finish;
   end
endmodule
