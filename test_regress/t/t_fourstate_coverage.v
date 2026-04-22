// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain.
// SPDX-FileCopyrightText: 2026 Antmicro
// SPDX-License-Identifier: CC0-1.0

module axi_adapter(
    input logic [1:0] priv_ids_i[1],
    input logic [1:0] priv_ids_i2[2][4]
);
    logic unused_sig;
    logic [3:0] exprstmt_lhs;
    logic [3:0] exprstmt_rhs;

    assign unused_sig = 1;

    always_comb begin
        exprstmt_lhs = 1;
        exprstmt_rhs = (exprstmt_lhs += 1);
    end

    interrupt xintr5(.clk(unused_sig));
endmodule

module interrupt(
    input logic clk
);
    /*verilator no_inline_module*/
    logic unused_interrupt_sig;

    assign unused_interrupt_sig = clk;
endmodule
