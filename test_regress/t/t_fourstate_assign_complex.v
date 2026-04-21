// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain.
// SPDX-FileCopyrightText: 2026 Antmicro
// SPDX-License-Identifier: CC0-1.0

class Base;
  logic [15:0] foo;
  function Base fooo();
    $c("std::ignore = 0;");
    return this;
  endfunction
endclass

class Derived extends Base;
  logic [15:0] bar;
endclass

module t;
  initial begin
    static Derived foo = new;
    foo.foo = 2;
    if (foo.fooo().foo !== 16'b0000000000000010) $stop;
    foo.foo[$c("0")] = 1;
    if (foo.foo !== 16'b0000000000000011) $stop;
    foo.foo[$c("3")+:2] = 2'd3;
    if (foo.foo !== 16'b0000000000011011) $stop;
    foo.fooo().foo[$c("3")+:2] = 2'bxz;
    if (foo.foo !== 16'b00000000000xz011) $stop;
    foo.foo[$c("10")-:2] = 2'bxz;
    if (foo.foo !== 16'b00000xz0000xz011) $stop;
    foo.foo[$c("100")-:2] = 2'bxz;
    if (foo.foo !== 16'b00000xz0000xz011) $stop;
    {foo.foo, foo.bar} = 32'h71209zx6;
    if (foo.foo !== 16'h7120) $stop;
    if (foo.bar !== 16'h9zx6) $stop;
    $write("*-* All Finished *-*\n" );
    $finish;
  end
endmodule
