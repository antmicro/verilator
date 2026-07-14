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

`define check_onehot(func) \
  begin \
    if ( $onehot(func('1))) $stop; \
    if (!$onehot(func('b01))) $stop; \
    if ( $onehot(func('b0))) $stop; \
    if ( $onehot(func('b0x))) $stop; \
    if ( $onehot(func('b0z))) $stop; \
    if (!$onehot(func('b1x))) $stop; \
    if (!$onehot(func('b1z))) $stop; \
    if ( $onehot(func('b0x))) $stop; \
    if ( $onehot(func('b0z))) $stop; \
    if (!$onehot(func('b10))) $stop; \
    if (!$onehot(func('b0x1))) $stop; \
    if (!$onehot(func('b0z1))) $stop; \
    if ( $onehot(func('b11))) $stop; \
  end

`define check_onehot0(func) \
  begin \
    if ( $onehot0(func('1))) $stop; \
    if (!$onehot0(func('b01))) $stop; \
    if (!$onehot0(func('0))) $stop; \
    if (!$onehot0(func('x))) $stop; \
    if (!$onehot0(func('z))) $stop; \
    if (!$onehot0(func('b1x))) $stop; \
    if (!$onehot0(func('b1z))) $stop; \
    if (!$onehot0(func('b0x))) $stop; \
    if (!$onehot0(func('b0z))) $stop; \
    if (!$onehot0(func('b10))) $stop; \
    if (!$onehot0(func('b0x1))) $stop; \
    if (!$onehot0(func('b0z1))) $stop; \
    if ( $onehot0(func('b11))) $stop; \
    if ( $onehot0(func('b1x1))) $stop; \
    if ( $onehot0(func('b1z1))) $stop; \
    if ( $onehot0(func('b101))) $stop; \
    if ( $onehot0(func('b111))) $stop; \
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
  logic [31:0] a;
  initial begin

    `check_onehot(f);
    `check_onehot(g);
    `check_onehot(h);
    `check_onehot(i);

    `check_onehot0(f);
    `check_onehot0(g);
    `check_onehot0(h);
    `check_onehot0(i);

    a = 'b1;
    if (!$onehot(a)) $stop;
    if (!$onehot0(a)) $stop;
    a = 'b0;
    if ( $onehot(a)) $stop;
    if (!$onehot0(a)) $stop;
    a = 'bx;
    if ( $onehot(a)) $stop;
    if (!$onehot0(a)) $stop;
    a = 'bz;
    if ( $onehot(a)) $stop;
    if (!$onehot0(a)) $stop;
    a = 'bx1;
    if (!$onehot(a)) $stop;
    if (!$onehot0(a)) $stop;
    a = 'b1x;
    if (!$onehot(a)) $stop;
    if (!$onehot0(a)) $stop;
    a = 'b1z;
    if (!$onehot(a)) $stop;
    if (!$onehot0(a)) $stop;
    a = 'bz1;
    if (!$onehot(a)) $stop;
    if (!$onehot0(a)) $stop;
    a = 'b11;
    if ( $onehot(a)) $stop;
    if ( $onehot(a)) $stop;
    a = 'b10;
    if (!$onehot(a)) $stop;
    if (!$onehot(a)) $stop;
    $write("*-* All Finished *-*\n");
    $finish;
  end
endmodule
