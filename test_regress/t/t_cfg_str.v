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

   // Scope configuration with encoded '\0'
   configure___0 ___0(clk);

    initial begin
      $write("*-* All Finished *-*\n");
      $finish;
   end
endmodule

module configure___0(input clk);
   //function void f_assert0; assert(0);
   //endfunction
   //function void f_assert_0; assert(0);
   //endfunction
   //function void f_assert__0; assert(0);
   //endfunction
   function void f_assert___0$; assert(0);
   endfunction

   initial begin
      // Assert scope with 0 bug
   //   f_assert0();
   //   f_assert_0();
   //   f_assert__0();
      f_assert___0$();
   end
endmodule
