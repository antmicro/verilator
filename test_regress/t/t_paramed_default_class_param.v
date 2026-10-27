// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain.
// SPDX-FileCopyrightText: 2026 Antmicro
// SPDX-License-Identifier: CC0-1.0

class Foo #(
    type T = bit
);
  function string name();
    return $typename(T);
  endfunction
endclass

class Bar #(
    type BASE = Foo#(logic)
) extends BASE;
endclass

// Default references another type parameter
class Baz #(
    type T = bit,
    type BASE = Foo#(T)
) extends BASE;
endclass

module t;
  Bar #(Foo#(logic)) a = new;
  Bar #(Foo#(bit)) b = new;
  Bar c;
  Baz #(bit, Foo#(bit)) d = new;
  Baz e;

  initial begin
    if (a.name() == b.name()) $stop;
    // An explicit default is the same type as the implicit one
    c = a;
    e = d;
    if (e.name() != "bit") $stop;
    $write("*-* All Finished *-*\n");
    $finish;
  end
endmodule
