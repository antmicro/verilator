// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain.
// SPDX-FileCopyrightText: 2026 PlanV GmbH
// SPDX-License-Identifier: CC0-1.0

// verilog_format: off
`define stop $stop
`define checkd(gotv,expv) do if ((gotv) !== (expv)) begin $write("%%Error: %s:%0d: %m got=%0d exp=%0d\n", `__FILE__,`__LINE__, (gotv), (expv)); `stop; end while(0)
// verilog_format: on

interface str_if;
  string s;
endinterface

module t;
  str_if sif ();
  virtual str_if vif = sif;
  string observed;
  int changes;
  assign observed = sif.s;
  always @(sif.s)++changes;

  initial begin
    #1;
    for (int i = 1; i < 5; ++i) begin
      automatic int old_changes = changes;
      vif.s = $sformatf("hello %0d", i);
      #1;
      `checkd(observed, $sformatf("hello %0d", i));
      `checkd(changes, old_changes + 1);
    end
    $write("*-* All Finished *-*\n");
    $finish;
  end
endmodule
