// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain.
// SPDX-FileCopyrightText: 2026 Antmicro Ltd
// SPDX-License-Identifier: CC0-1.0

// Test to assert that the bug with property argument type retained from
// the previous variable won't be present again

module t(input clk);
  genvar i;
  property prop(prop_arg);
    @(posedge clk)
    (prop_arg |-> prop_arg);
  endproperty

  wire w;
  property prop2(prop_arg);
    @(posedge clk)
    (prop_arg |-> prop_arg);
  endproperty

  initial begin
    $write("*-* All Finished *-*\n");
    $finish;
  end
endmodule
