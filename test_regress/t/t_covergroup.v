// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain, for
// any use, without warranty, 2025 by Antmicro.
// SPDX-License-Identifier: CC0-1.0

module t (/*AUTOARG*/);
   bit [1:0] a = 0;
   covergroup cg;
      coverpoint a;
   endgroup

   initial begin
      automatic cg cg_i = new;
      if (cg_i.get_inst_coverage() != 0) $stop;
      cg_i.sample();
      if (cg_i.get_inst_coverage() != 25) $stop;

      $write("*-* All Finished *-*\n");
      $finish;
   end
endmodule
