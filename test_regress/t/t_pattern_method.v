// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain, for
// any use, without warranty, 2024 by Antmicro.
// SPDX-License-Identifier: CC0-1.0

class Cls;
   function int get_first(int arg[2]);
   return arg[0];
   endfunction
endclass

module t (/*AUTOARG*/);

   initial begin
      Cls c = new;
      int a = c.get_first('{32'd1, 32'd2});
      if (a != 1) $stop;

      $write("*-* All Finished *-*\n");
      $finish;
   end

endmodule
