// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain, for
// any use, without warranty, 2025 by Antmicro.
// SPDX-License-Identifier: CC0-1.0

module t (/*AUTOARG*/
   // Outputs
   state,
   // Inputs
   clk
   );

   output state;
   input clk;

   reg [7:0] indexed;
   reg [3:0][7:0] \indexed_array ;
   reg [7:0] \complex.a_lt_b$in1 ;

   typedef struct packed {
      logic [7:0] \packed._$array ;
   } packed_struct;

   packed_struct[1:0] arr_p;
   
   initial begin
      $write("*-* All Finished *-*\n");
      $finish;
   end
endmodule
