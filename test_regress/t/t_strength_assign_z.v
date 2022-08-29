// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain, for
// any use, without warranty, 2022 by Antmicro Ltd.
// SPDX-License-Identifier: CC0-1.0

module t (/*AUTOARG*/);
   wire (supply0, supply1) a = 'z;
   wire (weak0, weak1) a = '1;

   always begin
      if (a === 1'b1) begin
         $write("*-* All Finished *-*\n");
         $finish;
      end
   end
endmodule
