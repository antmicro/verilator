// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain.
// SPDX-FileCopyrightText: 2026 Antmicro
// SPDX-License-Identifier: CC0-1.0

module t;
  logic [1:0] a;

  always @* begin
    case (a) inside
      2'b00: ;
      default: ;
    endcase
  end
endmodule
