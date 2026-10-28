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
  event changed;
  bit ready;
  task wait_ready();
    @(changed iff ready);
  endtask
  function void fire();
    ->changed;
  endfunction
endclass

module t;
  EventSource source;
  time wake_time;
  int static_wakes;
  int static_all_wakes;

  always @(source.changed iff source.ready) static_wakes++;
  always @(source.changed) static_all_wakes++;

  initial begin
    source = new();
    fork
      begin
        source.wait_ready();
        wake_time = $time;
      end
    join_none
    #1 source.fire();  // Not ready, so this fire is ignored
    #1 source.ready = 1;  // Must not wake on the ignored fire
    #1 source.fire();  // Wakes the waiter at t=3
    fork
      #1 source.fire();
    join_none
    @(source.changed iff source.ready);
    `checkd($time, 4);
    #1;
    `checkd(wake_time, 3);
    `checkd(static_wakes, 2);
    `checkd(static_all_wakes, 3);
    $write("*-* All Finished *-*\n");
    $finish;
  end
endmodule
