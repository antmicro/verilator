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
endclass

module t;
  initial begin
    static Derived foo = new;
    foo.fooo().foo = 2;
    if (foo.fooo().foo !== 16'b0000000000000010) $stop;
    $write("*-* All Finished *-*\n" );
    $finish;
  end
endmodule
