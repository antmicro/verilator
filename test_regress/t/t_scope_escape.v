// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain, for
// any use, without warranty, 2024 by Antmicro.
// SPDX-License-Identifier: CC0-1.0

module t (/*AUTOARG*/
   // Inputs
   clk
   );
   input clk;

   configure___0_ ___0_(clk);

    initial begin
      $write("*-* All Finished *-*\n");
      $finish;
   end
endmodule

module configure___0_(input clk);
   function void assert0; assert(0);
   endfunction
   function void assert_0; assert(0);
   endfunction
   // Scope with a __0 that should not be encoded as '\0'
   function void assert__0_; assert(0);
   endfunction
   // Scope with special characters
   function void assert___0$; assert(0);
   endfunction
   // Scope with characters resembling decoded '_'
   function void assert___05F; assert(0);
   endfunction
   // Scope with characters resembling decoded '_' and 0 after it
   function void assert___05F0; assert(0);
   endfunction

   initial begin
      assert0();
      assert_0();
      assert__0_();
      assert___0$();
      assert___05F();
      assert___05F0();
      $finish;
   end
endmodule
