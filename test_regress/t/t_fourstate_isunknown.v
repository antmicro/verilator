// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain
// SPDX-FileCopyrightText: 2026 Antmicro
// SPDX-License-Identifier: CC0-1.0

module t;
    bit sideEffect = 0;
    function logic bar();
        sideEffect = 1;
        return 'x;
    endfunction

  initial begin
    integer foo;
    if ($isunknown(foo) != '1) $stop;
    foo = $c(1);
    if ($isunknown(foo) != '0) $stop;
    foo = 'x;
    if ($isunknown(foo) != '1) $stop;
    foo = 'b001011z0;
    if ($isunknown(foo) != '1) $stop;
    foo = 'b001011x0;
    if ($isunknown(foo) != '1) $stop;
    foo = 17;
    if ($isunknown(foo) != '0) $stop;
    if (sideEffect != 0) $stop;
    if ($isunknown(bar()) != '1) $stop;
    if (sideEffect != 1) $stop;
    $write("*-* All Finished *-*\n");
    $finish;
  end
endmodule
