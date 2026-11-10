// DESCRIPTION: Verilator: Register indexed random object fields inside a guarded foreach
// SPDX-FileCopyrightText: 2026 The Verilator Authors
// SPDX-License-Identifier: CC0-1.0

class item;
  rand int value;
endclass
class holder;
  rand item slots[3];
  function new;
    foreach (slots[i]) slots[i] = new;
  endfunction
endclass
class packet;
  rand holder h = new;
  bit mode;
  constraint c {
    if (mode) foreach (h.slots[i]) h.slots[i].value == i + 1;
    else foreach (h.slots[i]) h.slots[i].value == i + 5;
  }
endclass
module t;
  initial begin
    automatic packet p = new;
    repeat (4) begin
      p.mode = !p.mode;
      if (p.randomize() != 1) $fatal;
      foreach (p.h.slots[i])
        if (p.h.slots[i].value != i + (p.mode ? 1 : 5)) $fatal;
    end
    $display("*-* All Finished *-*");
    $finish;
  end
endmodule
