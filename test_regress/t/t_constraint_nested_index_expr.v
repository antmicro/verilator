// DESCRIPTION: Verilator: Computed indices in nested class array constraints
// SPDX-FileCopyrightText: 2026 The Verilator Authors
// SPDX-License-Identifier: CC0-1.0

// verilog_format: off
`define stop $stop
`define checkd(gotv,expv) do if ((gotv) !== (expv)) begin $write("%%Error: %s:%0d:  got=%0d exp=%0d\n", `__FILE__,`__LINE__, (gotv), (expv)); `stop; end while(0);
// verilog_format: on

class item;
  rand int unsigned value;
  rand int unsigned other;
endclass

class holder;
  rand item slots[4];
  rand item dynamic_items[];
  rand int dynamic_values[];
  rand item queued_items[$];
  rand int queued_values[$];
  rand item associative_items[int];
  rand int associative_values[int];
  constraint pairs {
    dynamic_items.size() == 3;
    dynamic_values.size() == 2;
    queued_items.size() == 2;
    queued_values.size() == 2;
    foreach (dynamic_values[i]) {
      dynamic_values[i] == 17 + 3 * i;
      dynamic_values[i] == dynamic_items[i].value + 1;
    }
    foreach (associative_values[i]) {
      associative_values[i] == 60 + i;
      associative_values[i] == associative_items[i].value + 3;
    }
    foreach (queued_values[i]) {
      queued_values[i] == 34 + 3 * i;
      queued_values[i] == queued_items[i].other + 2;
    }
  }
  function new;
    foreach (slots[i]) slots[i] = new;
    dynamic_items = new[3];
    dynamic_values = new[2];
    associative_items[3] = new;
    associative_items[9] = new;
    associative_items[22] = null;
    associative_values[3] = 0;
    associative_values[9] = 0;
    for (int i = 0; i < 2; ++i) begin
      automatic item entry = new;
      dynamic_items[i] = new;
      queued_items.push_back(entry);
      queued_values.push_back(0);
    end
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
    foreach (c.h.dynamic_values[i]) begin
      `checkd(c.h.dynamic_values[i], 17 + 3 * i);
      `checkd(c.h.dynamic_items[i].value, 16 + 3 * i);
    end
    foreach (c.h.queued_values[i]) begin
      `checkd(c.h.queued_values[i], 34 + 3 * i);
      `checkd(c.h.queued_items[i].other, 32 + 3 * i);
    end
    foreach (c.h.associative_values[i]) begin
      `checkd(c.h.associative_values[i], 60 + i);
      `checkd(c.h.associative_items[i].value, 57 + i);
    end
    `checkd(c.h.dynamic_items[2] == null, 1);
    `checkd(c.h.associative_items[22] == null, 1);
    $display("*-* All Finished *-*");
    $finish;
  end
endmodule
