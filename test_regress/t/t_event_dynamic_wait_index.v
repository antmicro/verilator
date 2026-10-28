// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain.
// SPDX-FileCopyrightText: 2026 Antmicro
// SPDX-License-Identifier: CC0-1.0

// verilog_format: off
`define stop $stop
`define checkd(gotv,expv) do if ((gotv) !== (expv)) begin $write("%%Error: %s:%0d:  got=%0d exp=%0d\n", `__FILE__,`__LINE__, (gotv), (expv)); `stop; end while(0);
// verilog_format: on

`timescale 1ns / 1ps

class EventSource;
  event evs[2];
  int idx = 0;
  task wait_idx();
    @(evs[idx]);
  endtask
  function void fire(int i);
    ->evs[i];
  endfunction

  int old_wakes;
  int new_wakes;
  task check_late_selection();
    fork
      begin
        wait_idx();
        old_wakes++;
      end
    join_none
    #1;
    fire(1);
    idx = 1;
    fork
      begin
        @(evs[1]);
        new_wakes++;
      end
    join_none
    #1;
    `checkd(old_wakes, 0);
    `checkd(new_wakes, 0);
    fire(0);
    #1;
    `checkd(old_wakes, 1);
    `checkd(new_wakes, 0);
    fire(1);
    #1;
    `checkd(old_wakes, 1);
    `checkd(new_wakes, 1);
  endtask
endclass

module t;
  EventSource source;
  time wake_time;

  initial begin
    source = new();
    source.fire(1);  // Fired before anyone waits
    fork
      begin
        source.wait_idx();
        wake_time = $time;
      end
    join_none
    #1 source.idx = 1;
    #1 source.fire(1);
    #1;
    `checkd(wake_time, 0);
    source.fire(0);
    #1;
    `checkd(wake_time, 3);
    fork
      begin
        source.wait_idx();
        wake_time = $time;
      end
    join_none
    #1 source.fire(1);
    #1;
    `checkd(wake_time, 5);
    begin
      EventSource indexed;
      indexed = new;
      indexed.check_late_selection();
    end
    $write("*-* All Finished *-*\n");
    $finish;
  end
endmodule
