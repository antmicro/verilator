// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain.
// SPDX-FileCopyrightText: 2026 Antmicro
// SPDX-License-Identifier: CC0-1.0

// verilog_format: off
`define stop $stop
`define checkb(gotv,expv) do if ((gotv) !== (expv)) begin $write("%%Error: %s:%0d:  got='b%b exp='b%b\n", `__FILE__,`__LINE__, (gotv), (expv)); `stop; end while(0);
// verilog_format: on

module t;
  typedef struct packed {
    logic [3:0] data;
    logic       flag;
  } leaf_t;

  typedef struct packed {
    leaf_t      leaf;
    logic [1:0] tail;
  } root_t;

  initial begin
    leaf_t a;
    leaf_t b;
    root_t c;
    root_t d;

    a.data = 4'bxz01;
    a.flag = 1'bz;
    b = a;
    `checkb(b, a);
    `checkb(b.data, 4'bxz01);
    `checkb(b.flag, 1'bz);

    c.leaf = a;
    c.tail = 2'bx1;
    d = c;
    `checkb(d, c);
    `checkb(d.leaf.data, 4'bxz01);
    `checkb(d.leaf.flag, 1'bz);
    `checkb(d.tail, 2'bx1);

    $write("*-* All Finished *-*\n");
    $finish;
  end
endmodule
