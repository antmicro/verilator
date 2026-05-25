// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain
// SPDX-FileCopyrightText: 2026 Antmicro
// SPDX-License-Identifier: CC0-1.0

module t #(
) (
);
  logic a;
  logic [6:0] b;
  logic c;
  assign c = ({a, b} == 8'hFC);

  initial begin
    $write("*-* All Finished *-*\n");
    $finish;
  end
endmodule
