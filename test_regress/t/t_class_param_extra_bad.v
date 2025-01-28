// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain, for
// any use, without warranty, 2024 by Wilson Snyder.
// SPDX-License-Identifier: CC0-1.0

module t;

   class Cls1;
      typedef bit bool_t;
   endclass

   localparam Cls1#(123, integer, "text")::bool_t PARAM = 1;

endmodule
