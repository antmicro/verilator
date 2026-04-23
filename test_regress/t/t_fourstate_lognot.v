// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain.
// SPDX-FileCopyrightText: 2026 Antmicro
// SPDX-License-Identifier: CC0-1.0

module t;
  initial begin
    if ((!(0)) !== 1) $stop;
    if ((!(1)) !== 0) $stop;
    if ((!('x)) !== 'x) $stop;
    if ((!('z)) !== 'x) $stop;
    $write("*-* All Finished *-*\n");
    $finish;
  end
endmodule
