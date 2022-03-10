// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain, for
// any use, without warranty, 2022 by Krzysztof Bieganski.
// SPDX-License-Identifier: CC0-1.0

module t;
  int val;
  always @val $write("[%0t] val=%0d\n", $time, val);
  
  initial begin
    val = 1;
    #10 val = 2;
    fork #5 val = 4; join_none
    val = #10 val + 1;
    val <= #10 val + 2;
    fork #5 val = 6; join_none
    #20 $write("*-* All Finished *-*\n");
    $finish;
  end
endmodule
