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
    $write("%b\n", v);
    v = 'z;
    $write("%b\n", v);
    v = 0;
    $write("%b\n", v);
    v = 1;
    $write("%b\n", v);
    $write("%b\n", foo(1, 2));

    $write("%b\n%b\n", p, q);
    p = 'z;
    q = 'z;
    $write("%b\n%b\n", p, q);
    p = 1;
    q = 1;
    $write("%b\n%b\n", p, q);
    p = 0;
    q = 0;
    $write("%b\n%b\n", p, q);
    q = 32'b01x;
    $write("%b\n", q);
    q = 32'b01z;
    $write("%b\n", q);
    q = 32'b01xz;
    $write("%b\n", q);
    q = 32'b0101;
    $write("%b\n", q);
    q = 32'bxz;
    $write("%b\n", q);
    q = 32'bzx;
    $write("%b\n", q);

    $write("%b\n", o);
    o = 'z;
    $write("%b\n", o);
    o = 1;
    $write("%b\n", o);
    o = 0;
    $write("%b\n", o);
    o = 32'b01x;
    $write("%b\n", o);
    o = 32'b01z;
    $write("%b\n", o);
    o = 32'b01xz;
    $write("%b\n", o);
    o = 32'b0101;
    $write("%b\n", o);
    o = 32'bxz;
    $write("%b\n", o);
    o = 32'bzx;
    $write("%b\n", o);

    $write("*-* All Finished *-*\n");
    $finish;
  end
endmodule
