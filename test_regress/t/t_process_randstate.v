// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain, for
// any use, without warranty, 2023 by Antmicro Ltd.
// SPDX-License-Identifier: CC0-1.0

module t;
   bit b = 0;
   integer n, m;
   string s;
   process p;

   always @(b) begin
      if ($urandom % 2 == 0) $random;
      p.set_randstate(s);
   end

   initial begin
      p = process::self();
      s = p.get_randstate();
      n = $random;
      for (integer i = 0; i < 5; i++) begin
         b = ~b;
         #1; m = $random;
         if (n != m)
            $stop;
      end
      $write("*-* All Finished *-*\n");
      $finish;
   end
endmodule
