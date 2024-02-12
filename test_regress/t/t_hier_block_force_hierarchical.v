// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain, for
// any use, without warranty, 2024 by Antmicro Ltd.
// SPDX-License-Identifier: CC0-1.0

module t(
   clk
   );
   input clk;
   logic in1;
   logic out1;

   assign in1 = 0;

   Test test(.out (out1), .in (in1));

   always @ (posedge clk) begin
      if (out1 !== ~in1) $stop;
      $write("*-* All Finished *-*\n");
      $finish;
   end

endmodule

module Test
   (
   output out,
   input in
   );

   SubTest subTest(.out(out), .in(in));
endmodule

module SubTest
   (
   output out,
   input in
   );

   SubSubTest subSubTest(.out(out), .in(in));
endmodule

module SubSubTest
   (
   output out,
   input in
   );

   assign out = ~ in;
endmodule
