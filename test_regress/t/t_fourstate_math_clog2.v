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

module t;
  function integer f(integer x);
    if (`IMPURE_ONE) return x;
    return 'x;
  endfunction
  function logic [31:0] g(logic [31:0] x);
    if (`IMPURE_ONE) return x;
    return 'x;
  endfunction
  function logic [1:0] h(logic [1:0] x);
    if (`IMPURE_ONE) return x;
    return 'x;
  endfunction
  function logic [7:0] i(logic [7:0] x);
    if (`IMPURE_ONE) return x;
    return 'x;
  endfunction
  function logic [127:0] j(logic [127:0] x);
    if (`IMPURE_ONE) return x;
    return 'x;
  endfunction

  initial begin

    if ($clog2(f(11)) !== 4) $stop;
    if ($clog2(f('bx)) !== 'x) $stop;
    if ($clog2(f('bz)) !== 'x) $stop;
    if ($clog2(f('b101x)) !== 'x) $stop;
    if ($clog2(f('b101z)) !== 'x) $stop;

    if ($clog2(g(11)) !== 4) $stop;
    if ($clog2(g('bx)) !== 'x) $stop;
    if ($clog2(g('bz)) !== 'x) $stop;
    if ($clog2(g('b101x)) !== 'x) $stop;
    if ($clog2(g('b101z)) !== 'x) $stop;

    if ($clog2(h(2'b0)) !== 0) $stop;
    if ($clog2(h(2'b1)) !== 0) $stop;
    if ($clog2(h('x)) !== 'x) $stop;
    if ($clog2(h('z)) !== 'x) $stop;
    if ($clog2(h('b1x)) !== 'x) $stop;
    if ($clog2(h('b1z)) !== 'x) $stop;

    if ($clog2(i(0)) !== 0) $stop;
    if ($clog2(i(255)) !== 8) $stop;
    if ($clog2(i('x)) !== 'x) $stop;
    if ($clog2(i('z)) !== 'x) $stop;
    if ($clog2(i('b10xz01zx)) !== 'x) $stop;
    if ($clog2(i('b111x1111)) !== 'x) $stop;
    if ($clog2(i('b111z1111)) !== 'x) $stop;

    if ($clog2(j('0)) !== 0) $stop;
    if ($clog2(j('hffffffffffffffffffffffffffffffff)) !== 128) $stop;
    if ($clog2(j('x)) !== 'x) $stop;
    if ($clog2(j('z)) !== 'x) $stop;
    if ($clog2(j('hffffffffffffffxfffffffffffffffff)) !== 'x) $stop;
    if ($clog2(j('hffffffffffffffzfffffffffffffffff)) !== 'x) $stop;
    if ($clog2(j('hxffffffffffffffffffffffffffffffz)) !== 'x) $stop;

    // const eval
    if ($clog2(11) !== 4) $stop;
    if ($clog2('bx) !== 'x) $stop;
    if ($clog2('bz) !== 'x) $stop;
    if ($clog2('b101x) !== 'x) $stop;
    if ($clog2('b101z) !== 'x) $stop;
    if ($clog2('b10xz01zx) !== 'x) $stop;
    if ($clog2('hxffffffffffffffffffffffffffffffz) !== 'x) $stop;

    $write("*-* All Finished *-*\n");
    $finish;
  end
endmodule
