// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain, for
// any use, without warranty, 2025 by Antmicro.
// SPDX-License-Identifier: CC0-1.0

module t (clk);
   input clk;
   reg q, d, s, r;
   altos_dff_sr_err  g1 (q, clk, d, s, r);
endmodule

primitive altos_dff_sr_err (q, clk, d, s, r);
   output q;
   reg q;
   input clk, d, s, r;

   table
      ?   1 (0x)  ?   : ? : -;
      ?   0  ?   (0x) : ? : -;
      ?   0  ?   (x0) : ? : -;
      (0x) ?  0    0   : ? : 0;
      (0x) 1  x    0   : ? : 0;
      (0x) 0  0    x   : ? : 0;
      (1x) ?  0    0   : ? : 1;
      (1x) 1  x    0   : ? : 1;
      (1x) 0  0    x   : ? : 1;
   endtable
endprimitive
