// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain, for
// any use, without warranty, 2025 by Antmicro.
// SPDX-License-Identifier: CC0-1.0

class Cls1;
endclass

class Cls2 extends Cls1;
   rand int v;
   function new();
    v.rand_mode(0);
   endfunction
endclass

class Cls3 extends Cls1;
   rand Cls2 r;
   function new();
    r = new;
   endfunction
endclass

module t;
   Cls3 c = new;
   initial if(c.randomize() with {c.r.v inside {32'h0};} == 0) $finish;
endmodule
