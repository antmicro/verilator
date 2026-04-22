// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain.
// SPDX-FileCopyrightText: 2026 Antmicro
// SPDX-License-Identifier: CC0-1.0

`define stop $stop
`define checkd(gotv,expv) do if ((gotv) !== (expv)) begin $write("%%Error: %s:%0d:  got=%0d exp=%0d (%s !== %s)\n", `__FILE__,`__LINE__, (gotv), (expv), `"gotv`", `"expv`"); `stop; end while(0);

module t;
  initial begin
    `checkd($onehot(4'bxz00), 0);
    `checkd($onehot(4'bxz01), 1);
    `checkd($onehot(4'bxz10), 1);
    `checkd($onehot(4'bxz11), 0);
    `checkd($onehot(4'bxzxz), 0);
    `checkd($onehot0(4'bxz00), 1);
    `checkd($onehot0(4'bxz01), 1);
    `checkd($onehot0(4'bxz10), 1);
    `checkd($onehot0(4'bxz11), 0);
    `checkd($onehot0(4'bxzxz), 1);
    $write("*-* All Finished *-*\n");
    $finish;
  end
endmodule
