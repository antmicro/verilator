// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain.
// SPDX-FileCopyrightText: 2026 Antmicro
// SPDX-License-Identifier: CC0-1.0

module check #(
    parameter int Id = 0
) (
    input int value
);
endmodule

module leaf (
    input int value
);
endmodule

module mid;
  for (genvar i = 0; i < 2; ++i) begin : g
    leaf l (.value(10 + i));
  end
  leaf arr[1:0] (.value(20));
endmodule

module t;
  mid a ();
  mid b ();
  initial begin
    if (a.g[1].l.c.Id != 1 || a.g[1].l.c.value != 11) $stop;
    if (b.arr[0].c.Id != 2 || b.arr[0].c.value != 20) $stop;
    if (a.arr[1].w.Id != 3 || b.arr[1].w.Id != 3) $stop;
    $write("*-* All Finished *-*\n");
    $finish;
  end
endmodule

// Paths from the top module, in the compilation unit
bind t.a.g[1].l check #(.Id(1)) c (.*);
bind t.b.arr[0] check #(.Id(2)) c (.*);
// All elements of the arrays
bind leaf : t.a.arr, t.b.arr check #(.Id(3)) w (.value(value));
