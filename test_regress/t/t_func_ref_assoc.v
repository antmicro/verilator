// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain.
// SPDX-FileCopyrightText: 2026 The Verilator Authors
// SPDX-License-Identifier: CC0-1.0

class Collector;
  typedef struct {
    int count;
    byte unsigned data[$];
  } entry_t;
  entry_t entries[string];
  int counters[*];

  virtual function void update(ref entry_t item, const ref entry_t observed);
    item.count++;
    item.data.push_back(8'h81);
    if (observed.count != item.count || observed.data.size() != 1) $stop;
  endfunction

  task bump(ref int x);
    x += 7;
  endtask

  function void see(const ref int x);
    if (x != 0) $stop;
  endfunction

  function void sample(string key);
    update(entries[key], entries[key]);
  endfunction
endclass

module t;
  initial begin
    automatic Collector c = new;
    c.entries["old"].count = 8;
    c.sample("old");
    c.sample("new");
    if (c.entries.num() != 2 || c.entries["old"].count != 9 || c.entries["new"].count != 1)
      $stop;
    if (c.entries["new"].data[0] != 8'h81) $stop;
    c.bump(c.counters[73]);
    if (c.counters.num() != 1 || c.counters[73] != 7) $stop;
    c.see(c.counters[25]);
    if (!c.counters.exists(25)) $stop;
    $display("*-* All Finished *-*");
    $finish;
  end
endmodule
