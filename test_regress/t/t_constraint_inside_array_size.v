// DESCRIPTION: Verilator: Membership in a solver-sized array
//
// This file ONLY is placed under the Creative Commons Public Domain
// SPDX-FileCopyrightText: 2026 The Verilator Authors
// SPDX-License-Identifier: CC0-1.0

module t;
  class Sample;
    int length;
    bit expected;
    bit [7:0] search;
    rand bit [7:0] values[];
    rand bit [7:0] needle;
    constraint c {
      values.size() == length;
      foreach (values[i]) values[i] == 8'(i + 2);
      needle == search;
      (needle inside {values}) == expected;
    }
  endclass

  class MustContain;
    rand bit [7:0] values[$:2];
    rand bit [7:0] needle;
    constraint c {
      values.size() inside {[1:3]};
      needle inside {values};
      foreach (values[i]) values[i] == 8'h42;
    }
  endclass

  class Large;
    rand bit [7:0] values[];
    rand bit [7:0] queue[$:257];
    constraint c {
      values.size() == 257;
      8'h42 inside {values};
      queue.size() inside {[257:258]};
      8'h43 inside {queue};
    }
  endclass

  class VariableSize;
    rand bit [7:0] values[];
    rand bit [7:0] needle;
    constraint shape { values.size() inside {[1:3]}; }
    constraint lookup {
      needle inside {values};
      !(8'h11 inside {values});
      foreach (values[i]) values[i] == 8'h55;
    }
  endclass

  initial begin
    automatic Sample s = new;
    automatic MustContain m = new;
    automatic VariableSize v = new;
    automatic Large l = new;
    for (int n = 0; n <= 3; ++n) begin
      s.length = n;
      s.search = 2;
      s.expected = n != 0;
      if (s.randomize() != 1 || s.values.size() != n) $fatal;
      foreach (s.values[i]) if (s.values[i] != 8'(i + 2)) $fatal;
    end
    s.length = 1;
    s.search = 4;
    s.expected = 0;
    if (s.randomize() != 1 || s.values.size() != 1 || s.values[0] != 2) $fatal;
    s.values.rand_mode(0);
    s.search = 2;
    s.expected = 1;
    if (s.randomize() != 1 || s.needle != s.values[0]) $fatal;
    s.search = 7;
    s.expected = 0;
    if (s.randomize() != 1 || s.needle != 7 || s.values[0] != 2) $fatal;
    if (m.randomize() != 1 || !(m.values.size() inside {[1:3]}) || m.needle != 8'h42) $fatal;
    v.values = new[2];
    if (v.randomize() != 1 || !(v.values.size() inside {[1:3]}) || v.needle != 8'h55) $fatal;
    foreach (v.values[i]) if (v.values[i] != 8'h55) $fatal;
    if (l.randomize() != 1 || l.values.size() != 257 || !(8'h42 inside {l.values})
        || !(l.queue.size() inside {[257:258]}) || !(8'h43 inside {l.queue})) $fatal;
    $display("*-* All Finished *-*");
    $finish;
  end
endmodule
