// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain, for
// any use, without warranty, 2025 by Wilson Snyder.
// SPDX-License-Identifier: CC0-1.0

module t(/*AUTOARG*/);

   initial begin
      if (0) begin : block
      end
      disable block;
      $write("*-* All Finished *-*\n");
      $finish;
   end

endmodule
