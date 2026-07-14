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

`define check_func(func, bitwidth) \
  begin \
    if ($countones(func('0)) != 0) $stop; \
    if ($countones(func('1)) != bitwidth) $stop; \
    if ($countones(func('x)) != 0) $stop; \
    if ($countones(func('z)) != 0) $stop; \
    if ($countones(func('b10)) != 1) $stop; \
    if ($countones(func('b11)) != 2) $stop; \
    if ($countones(func('b1x)) != 1) $stop; \
    if ($countones(func('b1z)) != 1) $stop; \
    if ($countones(func('b0x1)) != 1) $stop; \
    if ($countones(func('b0z1)) != 1) $stop; \
    if ($countones(func('b11x01xz)) != 3) $stop; \
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

    `check_func(f, 32);
    `check_func(g, 32);
    `check_func(h, 8);
    `check_func(i, 128);

    $write("*-* All Finished *-*\n");
    $finish;
  end
endmodule
