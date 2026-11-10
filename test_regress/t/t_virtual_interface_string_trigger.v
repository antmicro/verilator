// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain.
// SPDX-FileCopyrightText: 2026 The Verilator Authors
// SPDX-License-Identifier: CC0-1.0

// verilog_format: off
`define stop $stop
`define checkd(gotv,expv) do if ((gotv) !== (expv)) begin $write("%%Error: %s:%0d:  got=%0d exp=%0d\n", `__FILE__, `__LINE__, (gotv), (expv)); `stop; end while(0);
// verilog_format: on

// Changing a string through a virtual interface must update dependent logic and
// wake a change monitor once, including when the new string has the same length.
interface str_if;
  string text;
endinterface

module t;
  str_if bus ();
  virtual str_if vif;
  bit is_cat;
  int changes;

  always @(bus.text) begin
    is_cat = (bus.text == "cat");
    ++changes;
  end

  initial begin
    vif = bus;
    #1;
    vif.text = "cat";
    #1;
    `checkd(is_cat, 1);
    `checkd(changes, 1);
    vif.text = "car";
    #1;
    `checkd(is_cat, 0);
    `checkd(changes, 2);
    vif.text = "car";
    #1;
    `checkd(changes, 2);
    vif.text = "";
    #1;
    `checkd(changes, 3);
    $write("*-* All Finished *-*\n");
    $finish;
  end
endmodule
