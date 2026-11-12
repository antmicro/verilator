// DESCRIPTION: Verilator: Reject constructor arguments that change a physical
// modport's view or its interface parameter when binding a virtual handle.
//
// This file ONLY is placed under the Creative Commons Public Domain.
// SPDX-FileCopyrightText: 2026 The Verilator Authors
// SPDX-License-Identifier: CC0-1.0

interface Link #(
    parameter int WIDTH = 4
);
  bit [WIDTH-1:0] value;
  modport Writer(output value);
  modport Reader(input value);
endinterface

class Channel #(
    type T = bit
);
  T selected;

  function new(T port);
    selected = port;
  endfunction
endclass

module t;
  Link ports[2] ();
  Channel #(virtual Link.Reader) wrong_view;
  Channel #(virtual Link) dropped_view;
  Channel #(virtual Link #(8).Reader) wrong_param;
  initial begin
    wrong_view = new(ports[0].Writer);
    dropped_view = new(t.ports[0].Reader);
    wrong_param = new(t.ports[1].Reader);
  end
`ifdef BAD_MODPORT_NAME
  // An unqualified physical instance cannot supply a nonexistent view.
  Channel #(virtual Link.Missing) missing_view;
  initial missing_view = new(t.ports[0]);
`endif
endmodule
