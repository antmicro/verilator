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
// verilog_format: off
`define stop $stop
`define checkd(gotv,expv) do if ((gotv) !== (expv)) begin $write("%%Error: %s:%0d:  got=%0d exp=%0d (%s !== %s)\n", `__FILE__,`__LINE__, (gotv), (expv), `"gotv`", `"expv`"); `stop; end while(0);
// verilog_format: on

`define IMPURE_N(n) (`IMPURE_ONE * (n))

class Base;
  logic [15:0] foo;
  int calls;
  function Base fooo();
    calls += 1;
    return this;
  endfunction
endclass

class Derived extends Base;
  logic [15:0] bar;
endclass

module t;
  function integer func();
    return `IMPURE_N(1);
  endfunction

  initial begin
    static Derived foo = new;
    foo.foo = 2;
    `checkd(foo.fooo().foo, 16'b0000000000000010);
    `checkd(foo.calls, 1);
    foo.foo[`IMPURE_N(0)] = 1;
    `checkd(foo.foo, 16'b0000000000000011);
    foo.foo[`IMPURE_N(3)+:2] = 2'd3;
    `checkd(foo.foo, 16'b0000000000011011);
    foo.fooo().fooo().foo[`IMPURE_N(3)+:2] = 2'bxz;
    `checkd(foo.calls, 3);
    `checkd(foo.foo, 16'b00000000000xz011);
    foo.foo[`IMPURE_N(10)-:2] = 2'bxz;
    `checkd(foo.foo, 16'b00000xz0000xz011);
    foo.foo[`IMPURE_N(100)-:2] = 2'bxz;
    `checkd(foo.foo, 16'b00000xz0000xz011);
    {foo.foo, foo.bar} = 32'h71209zx6;
    `checkd(foo.foo, 16'h7120);
    `checkd(foo.bar, 16'h9zx6);
    foo.fooo().foo[func()] = 1;
    `checkd(foo.calls, 4);
    `checkd(foo.foo, 16'h7122);
    $write("*-* All Finished *-*\n" );
    $finish;
  end
endmodule
