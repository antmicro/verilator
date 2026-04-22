// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain.
// SPDX-FileCopyrightText: 2026 Antmicro
// SPDX-License-Identifier: CC0-1.0

`define stop $stop
`define checkd(gotv,expv) do if ((gotv) !== (expv)) begin $write("%%Error: %s:%0d:  got=%0d exp=%0d (%s !== %s)\n", `__FILE__,`__LINE__, (gotv), (expv), `"gotv`", `"expv`"); `stop; end while(0);

module t;
  initial begin
    `checkd($countbits(4'bxz01, '1), 1);
    `checkd($countbits(4'bxz01, '0), 1);
    `checkd($countbits(4'bxx10, 'x), 2);
    `checkd($countbits(4'bzzz1, 'z), 3);
    `checkd($countbits(4'bxzxz, 'x, 'z), 4);
    `checkd($countbits(4'b1010, '1), 2);
    `checkd($countbits(4'b1010, '0), 2);
    `checkd($countbits(4'b1010, '1, '0), 4);
    `checkd($countbits(4'bxz01, 'x), 1);
    `checkd($countbits(4'bxz01, 'z), 1);
    `checkd($countbits(4'bxz01, '1, '0, 'x), 3);
    `checkd($countbits(4'b1100, '1), 2);
    `checkd($countbits(4'b1100, '0), 2);
    `checkd($countbits(4'b1100, '1, '0), 4);
    `checkd($countbits(4'b0000, '1), 0);
    `checkd($countbits(4'b0000, '0), 4);
    `checkd($countbits(4'b1111, '1), 4);
    `checkd($countbits(4'b1111, '0), 0);

    $write("*-* All Finished *-*\n");
    $finish;
  end
endmodule
