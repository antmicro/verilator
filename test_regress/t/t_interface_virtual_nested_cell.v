// DESCRIPTION: Verilator: Virtual interface to a cell nested beneath another module
// SPDX-FileCopyrightText: 2026 The Verilator Authors
// SPDX-License-Identifier: CC0-1.0

interface endpoint;
  int value;
endinterface

module unit;
  endpoint port ();
endmodule

module t;
  unit a (), b ();
  virtual endpoint selected;
  initial begin
    selected = a.port;
    selected.value = 17;
    selected = b.port;
    selected.value = 39;
    if (a.port.value != 17 || b.port.value != 39) $stop;
    $display("*-* All Finished *-*");
    $finish;
  end
endmodule
