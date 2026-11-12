// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain.
// SPDX-FileCopyrightText: 2026 Antmicro
// SPDX-License-Identifier: CC0-1.0

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
  Link #(8) single ();
endmodule

module t;
  leaf a ();
  Link #(8) local_ports[1:0] ();
  Link #(8) local_single ();
  Channel #(virtual Link #(8).Writer) c[4];
  initial begin
    c[0] = new(local_single.Writer);
    c[1] = new(local_ports[1].Writer);
    c[2] = new(a.single.Writer);
    c[3] = new(a.ports[3].Writer);
    foreach (c[i]) c[i].view.value = 8'(i + 1);
    if (local_single.value != 1 || local_ports[1].value != 2 || local_ports[0].value != 0) $stop;
    if (a.single.value != 3 || a.ports[3].value != 4 || a.ports[2].value != 0) $stop;
    $write("*-* All Finished *-*\n");
    $finish;
  end
endmodule
