// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain.
// SPDX-FileCopyrightText: 2026 Antmicro
// SPDX-License-Identifier: CC0-1.0

// verilog_format: off
`define stop $stop
`define checkd(gotv,expv) do if ((gotv) !== (expv)) begin $write("%%Error: %s:%0d:  got=%0d exp=%0d (%s !== %s)\n", `__FILE__,`__LINE__, (gotv), (expv), `"gotv`", `"expv`"); `stop; end while(0);
// verilog_format: on


`timescale 1ns / 1ps

class EventSource;
  event changed;
  task wait_once();
    @changed;
  endtask
  function void fire();
    ->changed;
  endfunction
endclass

class ReboundEventSource;
  event original;
  event replacement;
  event saved;
  int old_wakes;
  int new_wakes;

  task check_merge();
    saved = original;
    fork
      begin
        @original;
        old_wakes++;
      end
    join_none
    #1;
    original = replacement;
    ->replacement;
    fork
      begin
        @replacement;
        new_wakes++;
      end
    join_none
    #1;
    `checkd(old_wakes, 0);
    `checkd(new_wakes, 0);
    ->saved;
    #1;
    `checkd(old_wakes, 1);
    `checkd(new_wakes, 0);
    ->replacement;
    #1;
    `checkd(old_wakes, 1);
    `checkd(new_wakes, 1);
  endtask

  task check_trigger_before_merge();
    fork
      begin
        @original;
        old_wakes++;
      end
    join_none
    #1;
    ->original;
    original = replacement;
    #1;
    `checkd(old_wakes, 1);
  endtask

  task check_reference_release();
    EventSource selected;
    selected = new;
    saved = selected.changed;
    fork
      begin
        @(selected.changed);
        old_wakes++;
      end
    join_none
    #1;
    selected = null;
    ->saved;
    #1;
    `checkd(old_wakes, 1);
  endtask
endclass

module t;
  EventSource source;
  EventSource observed = new;
  int old_wakes;
  time old_wake_time;
  int static_wakes;

  always @(observed.changed) static_wakes++;

  initial begin
    source = new();
    fork
      begin
        source.wait_once();
        old_wakes++;
        old_wake_time = $time;
      end
    join_none
    #1;  // Main blocks, so the forked waiter starts and registers on @changed
    source.fire();  // Old waiter must wake at t=1
    fork
      #1 source.fire();  // Wakes main at t=2
    join_none
    source.wait_once();  // New waiter must not see the t=1 trigger
    `checkd($time, 2);
    `checkd(old_wakes, 1);
    `checkd(old_wake_time, 1);
    begin
      ReboundEventSource merged;
      merged = new;
      merged.check_merge();
    end
    begin
      ReboundEventSource merged;
      merged = new;
      merged.check_trigger_before_merge();
    end
    begin
      ReboundEventSource released;
      released = new;
      released.check_reference_release();
    end
    `checkd(static_wakes, 0);
    observed = source;
    #1;
    `checkd(static_wakes, 0);
    $write("*-* All Finished *-*\n");
    $finish;
  end
endmodule
