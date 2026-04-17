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
    $write("%h\n", v);
    v = 'z;
    $write("%h\n", v);
    v = 0;
    $write("%h\n", v);
    v = 1;
    $write("%h\n", v);
    $write("%h\n", foo(1, 2));

    $write("%h\n%h\n", p, q);
    p = 'z;
    q = 'z;
    $write("%h\n%h\n", p, q);
    p = 1;
    q = 1;
    $write("%h\n%h\n", p, q);
    p = 0;
    q = 0;
    $write("%h\n%h\n", p, q);
    q = 32'b01x;
    $write("%h\n", q);
    q = 32'b01z;
    $write("%h\n", q);
    q = 32'b01xz;
    $write("%h\n", q);
    q = 32'b0101;
    $write("%h\n", q);
    q = 32'bxz;
    $write("%h\n", q);
    q = 32'bzx;
    $write("%h\n", q);

    $write("%h\n", o);
    o = 'z;
    $write("%h\n", o);
    o = 1;
    $write("%h\n", o);
    o = 0;
    $write("%h\n", o);
    o = 32'b01x;
    $write("%h\n", o);
    o = 32'b01z;
    $write("%h\n", o);
    o = 32'b01xz;
    $write("%h\n", o);
    o = 32'b0101;
    $write("%h\n", o);
    o = 32'bxz;
    $write("%h\n", o);
    o = 32'bzx;
    $write("%h\n", o);
    o = 32'h908faczx;
    $write("%h\n", o);

    $write("*-* All Finished *-*\n");
    $finish;
  end
endmodule
