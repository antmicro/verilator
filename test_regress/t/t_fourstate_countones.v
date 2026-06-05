// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain
// SPDX-FileCopyrightText: 2026 Antmicro
// SPDX-License-Identifier: CC0-1.0

`ifdef VERILATOR
`define IMPURE_ONE ($c(1))
`else
`define IMPURE_ONE (|($random | $random))
`endif

module t;
  initial begin
    integer foo;
    if ($countones(foo) != 0) $stop;
    foo = `IMPURE_ONE;
    if ($countones(foo) != 1) $stop;
    foo = 'x;
    if ($countones(foo) != 0) $stop;
    foo = 'b001011z0;
    if ($countones(foo) != 3) $stop;
    foo = 'b001011x0;
    if ($countones(foo) != 3) $stop;
    foo = 17;
    if ($countones(foo) != 2) $stop;
    $write("*-* All Finished *-*\n");
    $finish;
  end
endmodule
