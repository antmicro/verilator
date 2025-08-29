// DESCRIPTION: Verilator: Verilog Test module for SystemVerilog 'alias'
//
// Simple bi-directional transitive alias test.
//
// This file ONLY is placed under the Creative Commons Public Domain, for
// any use, without warranty, 2013 by Jeremy Bennett.
// SPDX-License-Identifier: CC0-1.0

module t (/*AUTOARG*/
   // Inputs
   clk
   );
   input clk;

   wire [31:0] a;
   wire [31:0] b;
   wire [31:0] c;

   alias a = b = c;
   transfer transfer_i (.a (a),
                        .b (b),
                        .c (c));

   initial begin
      $write ("a = %x, b = %x, c = %x\n", a, b, c);
      if (a != 32'hdeadbeef) $stop;
      if (b != 32'hdeadbeef) $stop;
      if (c != 32'hdeadbeef) $stop;
      $write("*-* All Finished *-*\n");
      $finish;
   end

endmodule

module transfer (
   inout wire [31:0] a,
   inout wire [31:0] b,
   inout wire [31:0] c
   );

   assign b = 32'hdeadbeef;


endmodule
