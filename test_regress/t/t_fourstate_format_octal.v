// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain.
// SPDX-FileCopyrightText: 2026 Antmicro
// SPDX-License-Identifier: CC0-1.0

module t;
  logic p;
  integer unsigned q;
  integer o;

  function integer foo(integer a, integer b);
    return a + b;
  endfunction

  initial begin
    static logic v = 'x;
    $write("%o\n", v);
    v = 'z;
    $write("%o\n", v);
    v = 0;
    $write("%o\n", v);
    v = 1;
    $write("%o\n", v);
    $write("%o\n", foo(1, 2));

    $write("%o\n%o\n", p, q);
    p = 'z;
    q = 'z;
    $write("%o\n%o\n", p, q);
    p = 1;
    q = 1;
    $write("%o\n%o\n", p, q);
    p = 0;
    q = 0;
    $write("%o\n%o\n", p, q);
    q = 32'b01x;
    $write("%o\n", q);
    q = 32'b01z;
    $write("%o\n", q);
    q = 32'b01xz;
    $write("%o\n", q);
    q = 32'b0101;
    $write("%o\n", q);
    q = 32'bxz;
    $write("%o\n", q);
    q = 32'bzx;
    $write("%o\n", q);

    $write("%o\n", o);
    o = 'z;
    $write("%o\n", o);
    o = 1;
    $write("%o\n", o);
    o = 0;
    $write("%o\n", o);
    o = 32'b01x;
    $write("%o\n", o);
    o = 32'b01z;
    $write("%o\n", o);
    o = 32'b01xz;
    $write("%o\n", o);
    o = 32'b0101;
    $write("%o\n", o);
    o = 32'bxz;
    $write("%o\n", o);
    o = 32'bzx;
    $write("%o\n", o);
    o = 32'oz62z7x;
    $write("%o\n", o);

    $write("*-* All Finished *-*\n");
    $finish;
  end
endmodule
