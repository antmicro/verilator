// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain.
// SPDX-FileCopyrightText: 2026 Antmicro
// SPDX-License-Identifier: CC0-1.0

// With --threads, the two clocked blocks run in parallel 'nba' mtasks. First both fire
// events, which update the model's shared event state. Then the covergroup's event wait,
// started right in the constructor, reads the event counter while the other block fires.

class Source;
  event ev;
  task observe();
    forever @ev;
  endtask
endclass

class Monitor;
  event observed;
  bit value;
  covergroup cg @(observed);
    cp: coverpoint value;
  endgroup
  function new();
    cg = new;
  endfunction
endclass

module t;
  bit clk;
  Source a = new;
  Source b = new;
  Monitor monitor;

  always @(posedge clk) ->a.ev;
  always @(posedge clk) begin
    if ($time < 100) ->b.ev;
    else monitor = new;
  end

  initial a.observe();
  initial b.observe();

  initial begin
    repeat (200) #1 clk = ~clk;
    $write("*-* All Finished *-*\n");
    $finish;
  end
endmodule
