// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain.
// SPDX-FileCopyrightText: 2026 Antmicro
// SPDX-License-Identifier: CC0-1.0

// verilog_format: off
`define stop $stop
`define checkh(gotv,expv) do if ((gotv) !== (expv)) begin $write("%%Error: %s:%0d:  got=%0x exp=%0x (%s !== %s)\n", `__FILE__,`__LINE__, (gotv), (expv), `"gotv`", `"expv`"); `stop; end while(0);
`define checkd(gotv,expv) do if ((gotv) !== (expv)) begin $write("%%Error: %s:%0d:  got=%0d exp=%0d\n", `__FILE__,`__LINE__, (gotv), (expv)); `stop; end while(0);
// verilog_format: on

module t;
  typedef logic [129:0] wide;

  initial begin
    automatic integer foo[logic];
    automatic integer foo2[wide];
    automatic logic bar = 'x;
    automatic wide bar2 = 'x;
    foo['x] = 'b0x1;
    foo['z] = 'b0z1;
    foo[0] = 'b011;
    foo[1] = 'b0xz1;
    `checkd(foo[bar], 'b0x1);
    bar = 'z;
    `checkd(foo[bar], 'b0z1);
    bar = 0;
    `checkd(foo[bar], 'b011);
    bar = 1;
    `checkd(foo[bar], 'b0xz1);

    foo2[bar2] = 'b0x1;
    bar2[7] = 'z;
    foo2[bar2] = 'b0z1;
    bar2[7] = '0;
    foo2[bar2] = 'b011;
    bar2[7] = '1;
    foo2[bar2] = 'b0xz1;
    bar2[7] = 'x;
    `checkd(foo2[bar2], 'b0x1);
    bar2[7] = 'z;
    `checkd(foo2[bar2], 'b0z1);
    bar2[7] = 0;
    `checkd(foo2[bar2], 'b011);
    bar2[7] = 1;
    `checkd(foo2[bar2], 'b0xz1);

    $write("*-* All Finished *-*\n");
    $finish;
  end
endmodule
