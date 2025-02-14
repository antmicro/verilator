// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain, for
// any use, without warranty, 2025 by Antmicro.
// SPDX-License-Identifier: CC0-1.0

module t(clk);
input clk;
    reg in;
    reg out;
    dff (out, in, clk);
endmodule
primitive dff (out, in, clk);
    output out;
    reg out;
    input in, clk;
    table
        *  ?   : ? : 1;
        ? (01) : ? : 0;
    endtable
endprimitive
