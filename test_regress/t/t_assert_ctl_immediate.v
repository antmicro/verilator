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

   module_with_assert module_with_assert(clk);
   module_with_assertctl module_with_assertctl(clk);

   always @ (posedge clk) begin
      assert(0);
   end

   always @ (negedge clk) begin
      $write("*-* All Finished *-*\n");
      $finish;
   end
endmodule

module module_with_assert(input clk);
    always @(posedge clk) assert(0);
endmodule

module module_with_assertctl(input clk);
   function void assert_off; $assertoff;
   endfunction
   function void assert_on; $asserton;
   endfunction
   function void f_assert; assert(0);
   endfunction
   function void f_assert0; assert(0);
   endfunction
   function void f_assert_0; assert(0);
   endfunction
   function void f_assert__0; assert(0);
   endfunction
   function void f_assert___0; assert(0);
   endfunction

   initial begin
      assert_on();
      assert(0);
      assert_off();
      assert_off();
      assert(0);
      assert_on();
      assert_on();
      assert(0);

      f_assert();
      f_assert();
      assert_off();
      f_assert();
      f_assert();

      // Assert scope with 0 bug
      f_assert0();
      f_assert_0();
      f_assert__0();
      f_assert___0();
   end
endmodule
