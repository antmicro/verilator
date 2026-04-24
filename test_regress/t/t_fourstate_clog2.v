// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain.
// SPDX-FileCopyrightText: 2026 Antmicro
// SPDX-License-Identifier: CC0-1.0

`define stop $stop
`define checkb(gotv,expv) do if ((gotv) !== (expv)) begin $write("%%Error: %s:%0d:  got=%b exp=%b (%s !== %s)\n", `__FILE__,`__LINE__, (gotv), (expv), `"gotv`", `"expv`"); `stop; end while(0);

module t;
  initial begin
    `checkb($clog2(0), 32'd0);
    `checkb($clog2(1), 32'd0);
    `checkb($clog2(2), 32'd1);
    `checkb($clog2(3), 32'd2);
    `checkb($clog2(4), 32'd2);
    `checkb($clog2(5), 32'd3);
    `checkb($clog2(32'h7fff_ffff), 32'd31);
    `checkb($clog2(32'h8000_0000), 32'd31);
    `checkb($clog2(32'h8000_0001), 32'd32);

    `checkb($clog2(32'bx), 32'bx);
    `checkb($clog2(32'bz), 32'bx);
    `checkb($clog2(32'b10x), 32'bx);
    `checkb($clog2(32'b10z), 32'bx);

    $write("*-* All Finished *-*\n");
    $finish;
  end
endmodule
