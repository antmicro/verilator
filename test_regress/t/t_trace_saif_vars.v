// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed into the Public Domain, for any use,
// Author: Yu-Sheng Lin johnjohnlys@media.ee.ntu.edu.tw
// SPDX-License-Identifier: CC0-1.0

module t (/*AUTOARG*/
   // Outputs
   state,
   // Inputs
   clk
   );

   output [4:0] state;
   input clk;

   reg [3:0] indexed;
   reg [3:0][3:0] indexed_array;
   reg \dpath.a_lt_b$in1 ;

   typedef struct packed {
      logic [32:0] \packed._$array ;
   } packed_struct;

   packed_struct[4:0] arr_p;

   always @ (posedge clk)
   begin
      $finish;
   end

endmodule
