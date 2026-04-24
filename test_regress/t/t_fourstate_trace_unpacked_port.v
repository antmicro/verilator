// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain.
// SPDX-FileCopyrightText: 2026 Antmicro
// SPDX-License-Identifier: CC0-1.0

module t(
    input logic [31:0] priv_ids_i[4],
    input logic [31:0] priv_ids_2d_i[2][4]
);
    localparam int C_LAT = 0;
    localparam int DW = 32;

    logic [C_LAT+1:0] [DW-1:0] dp_rdata;
endmodule
