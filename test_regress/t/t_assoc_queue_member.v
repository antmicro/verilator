// DESCRIPTION: Verilator: Queue methods on an associative array in an object
// SPDX-FileCopyrightText: 2026 The Verilator Authors
// SPDX-License-Identifier: CC0-1.0

class store;
  int queues[int][$];
endclass

module t;
  initial begin
    automatic store s = new;
    if (s.queues[9].size() != 0 || s.queues.exists(9)) $fatal;
    s.queues[4].push_back(21);
    s.queues[4].push_front(12);
    if (s.queues.num() != 1 || s.queues[4].size() != 2) $fatal;
    if (s.queues[4].pop_back() != 21) $fatal;
    if (s.queues[4][0] != 12) $fatal;
    s.queues[4].delete();
    if (s.queues[4].size() != 0 || !s.queues.exists(4)) $fatal;
    $display("*-* All Finished *-*");
    $finish;
  end
endmodule
