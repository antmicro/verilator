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

`define check_func(func) \
  begin \
    if ( $isunknown(func('0))) $stop; \
    if ( $isunknown(func('1))) $stop; \
    if (!$isunknown(func('x))) $stop; \
    if (!$isunknown(func('z))) $stop; \
    if (!$isunknown(func('b11x01xz))) $stop; \
    if (!$isunknown(func('b111011z))) $stop; \
    if (!$isunknown(func('b11x01x0))) $stop; \
    if ( $isunknown(func('b1110110))) $stop; \
  end

module t;
  function integer f(integer x);
    if (`IMPURE_ONE) return x;
    return 'x;
  endfunction
  function logic [31:0] g(logic [31:0] x);
    if (`IMPURE_ONE) return x;
    return 'x;
  endfunction
  function logic [7:0] h(logic [7:0] x);
    if (`IMPURE_ONE) return x;
    return 'x;
  endfunction
  function logic [127:0] i(logic [127:0] x);
    if (`IMPURE_ONE) return x;
    return 'x;
  endfunction

  bit sideEffect = 0;
  function logic impureFunc1();
    sideEffect = 1;
    return 'x;
  endfunction
  function logic impureFunc2();
    sideEffect = 1;
    return '0;
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

    `check_func(f);
    `check_func(g);
    `check_func(h);
    `check_func(i);

    sideEffect = 0;
    if (!`IMPURE_ONE && $isunknown(impureFunc1()) != '1) $stop;
    if (sideEffect != 0) $stop;
    if (`IMPURE_ONE && $isunknown(impureFunc1()) != '1) $stop;
    if (sideEffect != 1) $stop;

    sideEffect = 0;
    if (!`IMPURE_ONE && $isunknown(impureFunc2())) $stop;
    if (sideEffect != 0) $stop;
    if (`IMPURE_ONE && $isunknown(impureFunc2())) $stop;
    if (sideEffect != 1) $stop;

    $write("*-* All Finished *-*\n");
    $finish;
  end
endmodule
