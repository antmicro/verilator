// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain.
// SPDX-FileCopyrightText: 2026 Wilson Snyder
// SPDX-License-Identifier: CC0-1.0

// Resolve a hierarchical driver of an inout port alongside a clocking output.
// Check the external values before the first clocking drive and the clocking
// values after the external driver releases the port.
interface pins (
    input bit clk,
    inout wire value
);
  clocking cb @(posedge clk);
    output #0 value;
  endclocking
endinterface

module t;
  bit clk;
  bit enable = 1;
  bit value = 1;
  pins port (clk,);
  assign port.value = enable ? value : 1'bz;
  tri bus;
  bit parent_en, child_en;
  pins connected (clk, bus);
  assign bus = parent_en ? 1'b1 : 1'bz;
  assign connected.value = child_en ? 1'b0 : 1'bz;
  always #5 clk = !clk;

  initial begin
    #1;
    if (port.value !== 1) $stop;
    value = 0;
    #1;
    if (port.value !== 0) $stop;
    value = 1;
    #1;
    if (port.value !== 1) $stop;
    @(port.cb);
    enable = 0;
    port.cb.value <= 0;
    #1;
    if (port.value !== 0) $stop;
    @(port.cb);
    port.cb.value <= 1;
    #1;
    if (port.value !== 1) $stop;
    parent_en = 1;
    #1;
    if (bus !== 1 || connected.value !== 1) $stop;
    parent_en = 0;
    child_en = 1;
    #1;
    if (bus !== 0 || connected.value !== 0 || port.value !== 1) $stop;
    child_en = 0;
    parent_en = 1;
    #1;
    if (bus !== 1 || connected.value !== 1) $stop;
    $display("*-* All Finished *-*");
    $finish;
  end
endmodule
