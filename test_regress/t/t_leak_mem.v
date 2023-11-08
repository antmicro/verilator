// DESCRIPTION: Verilator: memory management test
//
// This file ONLY is placed under the Creative Commons Public Domain, for
// any use, without warranty, 2023 by Antmicro Ltd.
// SPDX-License-Identifier: CC0-1.0

typedef class Cls;

class Ref;
   Ref r;

   function void set(Ref obj);
      r = obj;
   endfunction

   function Ref get();
      return r;
   endfunction
endclass

module t;
   Ref cycle;

   initial begin
      for (integer i = 0; i < 100000; i++) begin
         cycle = new;
         cycle.set(cycle);
      end
      $write("*-* All Finished *-*\n");
      $finish;
   end
endmodule
