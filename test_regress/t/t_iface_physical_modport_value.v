// DESCRIPTION: Verilator: Pass an indexed physical modport to a virtual-interface
// constructor and verify that writes reach only the selected instance.
//
// This file ONLY is placed under the Creative Commons Public Domain.
// SPDX-FileCopyrightText: 2026 The Verilator Authors
// SPDX-License-Identifier: CC0-1.0

// verilog_format: off
`define stop $stop
`define checkh(gotv,expv) do if ((gotv) !== (expv)) begin $write("%%Error: %s:%0d:  got=%0x exp=%0x (%s !== %s)\n", `__FILE__,`__LINE__, (gotv), (expv), `"gotv`", `"expv`"); `stop; end while(0);
// verilog_format: on

interface Link #(
    parameter int WIDTH = 4
);
  bit [WIDTH-1:0] value;
  modport Writer(output value);
endinterface

class Channel #(
    type T = bit
);
  T view;
  function new(T port);
    view = port;
  endfunction
endclass

module leaf;
  Link #(8) ports[3:2] ();
endmodule

module t;
  leaf a (), b ();
  Channel #(virtual Link #(8).Writer) first;
  Channel #(virtual Link #(8).Writer) second;
  initial begin
    first = new(t.a.ports[3].Writer);
    second = new(t.b.ports[2].Writer);
    first.view.value = 8'h3a;
    second.view.value = 8'h2b;
    `checkh(a.ports[3].value, 8'h3a);
    `checkh(a.ports[2].value, 8'h00);
    `checkh(b.ports[2].value, 8'h2b);
    `checkh(b.ports[3].value, 8'h00);
    $write("*-* All Finished *-*\n");
    $finish;
  end
endmodule
