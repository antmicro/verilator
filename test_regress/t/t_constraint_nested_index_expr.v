// DESCRIPTION: Verilator: Computed indices in nested class array constraints
// SPDX-FileCopyrightText: 2026 The Verilator Authors
// SPDX-License-Identifier: CC0-1.0

class item;
  rand int unsigned value;
  rand int unsigned other;
endclass

class holder;
  rand item slots[4];
  function new;
    foreach (slots[i]) slots[i] = new;
  endfunction
endclass

class collection;
  rand holder h;
  int offset;
  function new;
    h = new;
  endfunction
  constraint c {
    h.slots[offset+1].value == 10;
    h.slots[offset+2].value + h.slots[offset+1].value == 35;
    foreach (h.slots[i])
      if (i > 0) h.slots[i-1].other + h.slots[i-1].value == 40 + i;
  }
endclass

module t;
  initial begin
    automatic collection c = new;
    for (int j = 0; j < 2; ++j) begin
      c.offset = j;
      if (c.randomize() == 0) $fatal;
      if (c.h.slots[j+1].value != 10 || c.h.slots[j+2].value != 25) $fatal;
      for (int i = 1; i < 4; ++i)
        if (c.h.slots[i-1].other + c.h.slots[i-1].value != 40 + i) $fatal;
    end
    $display("*-* All Finished *-*");
    $finish;
  end
endmodule
