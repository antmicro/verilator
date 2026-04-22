// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain.
// SPDX-FileCopyrightText: 2026 Antmicro
// SPDX-License-Identifier: CC0-1.0

`define EXPECT(x, e) if (x !== e) begin $write("Expected: %b, but got: %b\n", e, x); $stop; end

module t (
  input clk
);
  initial begin
    `EXPECT(clk, 'z);
    #100 `EXPECT(clk, 'z);
    $write("*-* All Finished *-*\n");
    $finish;
  end
endmodule
