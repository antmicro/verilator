// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain
// SPDX-FileCopyrightText: 2026 The Verilator Authors
// SPDX-License-Identifier: CC0-1.0

interface chan;
  typedef struct packed { bit [79:0] data; bit tag; } entry_t;
  entry_t entries[$];
  int items[];
  function automatic void push(entry_t entry);
    entries.push_back(entry);
  endfunction
endinterface

module t;
  chan bus();
  virtual chan vif;
  task automatic check(int count, bit [79:0] front, int first_value);
    if (bus.entries.size() != count || bus.items.size() != count) $stop;
    if (count != 0 && (bus.entries[0].data != front || bus.items[0] != first_value)) $stop;
  endtask

  initial begin
    vif = bus;
    #1;
    vif.push('{80'hcafebabedeadbeefabcd, 1'b1});
    vif.items = new[1];
    vif.items[0] = 17;
    #1;
    check(1, 80'hcafebabedeadbeefabcd, 17);
    vif.entries[0].data = 80'h4321;
    vif.items[0] = 29;
    #1;
    check(1, 80'h4321, 29);
    vif.entries.delete();
    vif.items.delete();
    #1;
    check(0, 0, 0);
    $write("*-* All Finished *-*\n");
    $finish;
  end
endmodule
