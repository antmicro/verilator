// DESCRIPTION: Verilator: Owner-bound task writes and suspended method receivers
//
// This file ONLY is placed under the Creative Commons Public Domain.
// SPDX-FileCopyrightText: 2026 Wilson Snyder
// SPDX-License-Identifier: CC0-1.0

// Check owner selection for class-task writes and suspended tasks.
// Distinct owners expose incorrect receivers and missing scheduling dependencies.

// verilog_format: off
`define stop $stop
`define checkd(gotv,expv) do if ((gotv) !== (expv)) begin $write("%%Error: %s:%0d: %m got=%0d exp=%0d\n", `__FILE__,`__LINE__, (gotv), (expv)); `stop; end while(0)
// verilog_format: on

interface task_owner_if;
  int x;
  int y;
  class Writer;
    task copy();
      // Both accesses use the declaring interface's capture, including the write.
      y = x;
    endtask
  endclass
  Writer c = new;
endinterface

module task_owner_case (
    output bit done
);
  task_owner_if first (), second ();
  initial begin
    first.x = 1;
    second.x = 2;
    #1;
    first.c.copy();
    // A write through one object's capture must leave the other owner unchanged.
    `checkd(first.y, 1);
    `checkd(second.y, 0);
    second.c.copy();
    `checkd(first.y, 1);
    `checkd(second.y, 2);
    first.x = 11;
    second.x = 22;
    first.c.copy();
    second.c.copy();
    `checkd(first.y, 11);
    `checkd(second.y, 22);
    done = 1;
  end
endmodule

interface timed_task_if (
    input logic clk
);
  logic [7:0] data;
  bit valid;
  class Driver;
    task send_data(logic [7:0] value);
      // Suspension must preserve the owner receiver for writes after both clock waits.
      @(posedge clk);
      data = value;
      valid = 1;
      @(posedge clk);
      data = '0;
      valid = 0;
    endtask
  endclass
  Driver drv;
  initial drv = new;
endinterface

module timed_task_case (
    output bit done
);
  bit clk = 0;
  always #5 clk = ~clk;
  timed_task_if first (clk), second (clk);
  bit [7:0] first_data[$], second_data[$];
  // Sample between drive edges so checks do not depend on active-region ordering.
  always @(negedge clk) begin
    if (first.valid) first_data.push_back(first.data);
    if (second.valid) second_data.push_back(second.data);
  end
  initial begin
    #1;
    while (first.drv == null || second.drv == null) @(posedge clk);
    first.drv.send_data(8'haa);
    repeat (5) @(posedge clk);
    second.drv.send_data(8'hbb);
    #1;
    // Indirect writes must reach the monitors, with exactly one transfer per owner.
    `checkd(first_data.size(), 1);
    `checkd(second_data.size(), 1);
    `checkd(first_data[0], 8'haa);
    `checkd(second_data[0], 8'hbb);
    `checkd(first.data, 0);
    `checkd(second.data, 0);
    `checkd(first.valid, 0);
    `checkd(second.valid, 0);
    done = 1;
  end
endmodule

module t;
  wire [1:0] done;
  task_owner_case task_owners (done[0]);
  timed_task_case timed_tasks (done[1]);

  initial begin
    wait (&done);
    $write("*-* All Finished *-*\n");
    $finish;
  end
  initial begin
    #100;
    $error("Incomplete scenarios: done=%b", done);
    `stop;
  end
endmodule
