// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain.
// SPDX-FileCopyrightText: 2026 Antmicro
// SPDX-License-Identifier: CC0-1.0

`ifdef VERILATOR
`define IMPURE_ONE ($c(1))
`else
`define IMPURE_ONE (|($random | $random))
`endif

`define check_one_arg(func, bitwidth) \
  begin \
    if ($countbits(func('b11x01xz), '0) != (bitwidth - 6)) $stop; \
    if ($countbits(func('b11x01xz), '1) != 3) $stop; \
    if ($countbits(func('b11x01xz), 'x) != 2) $stop; \
    if ($countbits(func('b11x01xz), 'z) != 1) $stop; \
    if ($countbits(func('b11x01xz), b1_0) != (bitwidth - 6)) $stop; \
    if ($countbits(func('b11x01xz), b1_1) != 3) $stop; \
    if ($countbits(func('b11x01xz), b1_x) != 2) $stop; \
    if ($countbits(func('b11x01xz), b1_z) != 1) $stop; \
    if ($countbits(func('b11x01xz), b32_0) != (bitwidth - 6)) $stop; \
    if ($countbits(func('b11x01xz), b32_1) != 3) $stop; \
    if ($countbits(func('b11x01xz), b32_x) != 2) $stop; \
    if ($countbits(func('b11x01xz), b32_z) != 1) $stop; \
    if ($countbits(func('b11x01xz), b128_0) != (bitwidth - 6)) $stop; \
    if ($countbits(func('b11x01xz), b128_1) != 3) $stop; \
    if ($countbits(func('b11x01xz), b128_x) != 2) $stop; \
    if ($countbits(func('b11x01xz), b128_z) != 1) $stop; \
    if ($countbits(func('b11x01xz), fake0b2_1) != (bitwidth - 6)) $stop; \
    if ($countbits(func('b11x01xz), fake0b2_x) != (bitwidth - 6)) $stop; \
    if ($countbits(func('b11x01xz), fake0b2_z) != (bitwidth - 6)) $stop; \
  end

`define check_two_args(func, bitwidth) \
  begin \
    if ($countbits(func('b11x01xz), '0, '1) != (bitwidth - 2 - 1)) $stop; \
    if ($countbits(func('b11x01xz), '0, 'x) != (bitwidth - 3 - 1)) $stop; \
    if ($countbits(func('b11x01xz), '0, 'z) != (bitwidth - 3 - 2)) $stop; \
    if ($countbits(func('b11x01xz), '1, 'x) != 5) $stop; \
    if ($countbits(func('b11x01xz), '1, 'z) != 4) $stop; \
    if ($countbits(func('b11x01xz), 'x, 'z) != 3) $stop; \
  end

`define check_three_args(func, bitwidth) \
  begin \
    if ($countbits(func('b11x01xz), '0, '1, 'x) != (bitwidth - 1)) $stop; \
    if ($countbits(func('b11x01xz), '0, '1, 'z) != (bitwidth - 2)) $stop; \
    if ($countbits(func('b11x01xz), '0, 'x, 'z) != (bitwidth - 3)) $stop; \
    if ($countbits(func('b11x01xz), '1, 'x, 'z) != 6) $stop; \
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

  logic [31:0] a = 'b11x01xz;
  logic [31:0] no0 = 'hffffffxz;
  logic [31:0] no1 = 'bxz;
  logic [31:0] nox = 'b1z;
  logic [31:0] noz = 'bx1;

  logic b1_0 = '0;
  logic b1_1 = '1;
  logic b1_x = 'x;
  logic b1_z = 'z;
  logic [31:0] b32_0 = '0;
  logic [31:0] b32_1 = '1;
  logic [31:0] b32_x = 'x;
  logic [31:0] b32_z = 'z;
  logic [127:0] b128_0 = '0;
  logic [127:0] b128_1 = '1;
  logic [127:0] b128_x = 'x;
  logic [127:0] b128_z = 'z;
  logic [1:0] fake0b2_1 = 2'('b10);
  logic [1:0] fake0b2_x = 2'('bx0);
  logic [1:0] fake0b2_z = 2'('bz0);
  integer bitwidth = 128;

  initial begin
    fake0b2_1 = 2'('b10);
    fake0b2_x = 2'('bx0);
    fake0b2_z = 2'('bz0);

    if ($countbits('b11x01xz, '0) != 26) $stop;
    if ($countbits('b11x01xz, '1) != 3) $stop;
    if ($countbits('b11x01xz, 'x) != 2) $stop;
    if ($countbits('b11x01xz, 'z) != 1) $stop;

    if ($countbits('b11x01xz, '0, '1) != 29) $stop;
    if ($countbits('b11x01xz, '0, 'x) != 28) $stop;
    if ($countbits('b11x01xz, '0, 'z) != 27) $stop;
    if ($countbits('b11x01xz, '1, 'x) != 5) $stop;
    if ($countbits('b11x01xz, '1, 'z) != 4) $stop;
    if ($countbits('b11x01xz, 'x, 'z) != 3) $stop;

    if ($countbits('b11x01xz, '0, '1, 'x) != 31) $stop;
    if ($countbits('b11x01xz, '0, '1, 'z) != 30) $stop;
    if ($countbits('b11x01xz, '0, 'x, 'z) != 29) $stop;
    if ($countbits('b11x01xz, '1, 'x, 'z) != 6) $stop;

    `check_one_arg(f, 32);
    `check_one_arg(g, 32);
    `check_one_arg(h, 8);
    `check_one_arg(i, 128);

    `check_two_args(f, 32);
    `check_two_args(g, 32);
    `check_two_args(h, 8);
    `check_two_args(i, 128);

    `check_three_args(f, 32);
    `check_three_args(g, 32);
    `check_three_args(h, 8);
    `check_three_args(i, 128);

    if ($countbits(a, '0) != 26) $stop;
    if ($countbits(a, '1) != 3) $stop;
    if ($countbits(a, 'x) != 2) $stop;
    if ($countbits(a, 'z) != 1) $stop;

    if ($countbits(a, '0, '1) != 29) $stop;
    if ($countbits(a, '0, 'x) != 28) $stop;
    if ($countbits(a, '0, 'z) != 27) $stop;
    if ($countbits(a, '1, 'x) != 5) $stop;
    if ($countbits(a, '1, 'z) != 4) $stop;
    if ($countbits(a, 'x, 'z) != 3) $stop;

    if ($countbits(a, '0, '1, 'x) != 31) $stop;
    if ($countbits(a, '0, '1, 'z) != 30) $stop;
    if ($countbits(a, '0, 'x, 'z) != 29) $stop;
    if ($countbits(a, '1, 'x, 'z) != 6) $stop;

    $write("*-* All Finished *-*\n");
    $finish;
  end
endmodule
