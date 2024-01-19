// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain, for
// any use, without warranty, 2024 by Antmicro Ltd.
// SPDX-License-Identifier: CC0-1.0

module t();
    reg in;
    wire out;
    assign in = 1'b0;
    Test test(.in (in), .out (out));

    initial begin
        if (in !== ~out) $display("in (0x%X) !== ~out (~ 0x%X)", in, out);
        else $write("*-* All Finished *-*\n");
        $finish;
    end
endmodule

module Test (input reg in, output wire out); /*verilator hier_block*/
    assign out = ~in;
endmodule
