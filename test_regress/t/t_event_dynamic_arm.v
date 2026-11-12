// DESCRIPTION: Verilator: Dynamic named event waits
//
// This file ONLY is placed under the Creative Commons Public Domain.
// SPDX-FileCopyrightText: 2026-2026 Wilson Snyder
// SPDX-License-Identifier: CC0-1.0

module t;
  class Monitor;
    event wake;
    int seen;
    bit enabled;

    task wait_enabled;
      @(wake iff enabled);
      seen |= 4;
    endtask

    function void start_waiter(int mask);
      fork
        begin
          @wake;
          seen |= mask;
        end
      join_none
    endfunction
  endclass

  Monitor monitor = new;

  initial begin
    monitor.start_waiter(1);
    fork
      monitor.wait_enabled();
    join_none
    #1;
    ->monitor.wake;
    monitor.start_waiter(2);
    #1;
    if (monitor.seen != 1) $stop;
    monitor.enabled = 1;
    #1;
    if (monitor.seen != 1) $stop;
    ->monitor.wake;
    #1;
    if (monitor.seen != 7) $stop;
    $write("*-* All Finished *-*\n");
    $finish;
  end
endmodule
