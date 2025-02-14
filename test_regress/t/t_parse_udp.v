// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain, for
// any use, without warranty, 2025 by Antmicro.
// SPDX-License-Identifier: CC0-1.0

module t (clk);
   input clk;
   reg q0;
   reg d0;
   reg q1;
   reg d1;
   reg q2;
   reg d2;

   level_symbols level_symbols(q0, clk, d0);
   edge_symbols edge_symbols(q1, clk, d1);
   output_symbols output_symbols(q2, clk, d2);
endmodule

primitive level_symbols (q, clk, d);
   output q;
   reg q;
   input clk, d;

   table
      (00) ? : ? : -;
      (01) ? : ? : -;
      (10) ? : ? : -;
      (11) ? : ? : -;

      (xx) ? : ? : -;
      (x0) ? : ? : -;
      (0x) ? : ? : -;
      (x1) ? : ? : -;
      (1x) ? : ? : -;

      (XX) ? : ? : -;
      (xX) ? : ? : -;
      (Xx) ? : ? : -;
      (X0) ? : ? : -;
      (0X) ? : ? : -;
      (X1) ? : ? : -;
      (1X) ? : ? : -;

      (??) ? : ? : -;
      (0?) ? : ? : -;
      (?0) ? : ? : -;
      (1?) ? : ? : -;
      (?1) ? : ? : -;
      (x?) ? : ? : -;
      (?x) ? : ? : -;
      (X?) ? : ? : -;
      (?X) ? : ? : -;

      (bb) ? : ? : -;
      (b0) ? : ? : -;
      (0b) ? : ? : -;
      (b1) ? : ? : -;
      (1b) ? : ? : -;
      (bx) ? : ? : -;
      (xb) ? : ? : -;
      (bX) ? : ? : -;
      (Xb) ? : ? : -;
      (b?) ? : ? : -;
      (?b) ? : ? : -;

      (Bb) ? : ? : -;
      (bB) ? : ? : -;
      (BB) ? : ? : -;
      (B0) ? : ? : -;
      (0B) ? : ? : -;
      (B1) ? : ? : -;
      (1B) ? : ? : -;
      (Bx) ? : ? : -;
      (xB) ? : ? : -;
      (BX) ? : ? : -;
      (XB) ? : ? : -;
      (B?) ? : ? : -;
      (?B) ? : ? : -;
   endtable
endprimitive

primitive edge_symbols (q, clk, d);
   output q;
   reg q;
   input clk, d;

   table
      r ? : ? : -;
      R ? : ? : -;
      f ? : ? : -;
      F ? : ? : -;
      p ? : ? : -;
      P ? : ? : -;
      n ? : ? : -;
      N ? : ? : -;
      * ? : ? : -;
   endtable
endprimitive

primitive output_symbols (q, clk, d);
   output q;
   reg q;
   input clk, d;

   table
      ? ? : ? : 0;
      ? ? : ? : 1;
      ? ? : ? : x;
      ? ? : ? : X;
      ? ? : ? : -;
   endtable
endprimitive
