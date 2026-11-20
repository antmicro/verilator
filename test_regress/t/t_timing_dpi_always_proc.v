// DESCRIPTION: Verilator: Non-context DPI import with a process-dependent export
//
// This file ONLY is placed under the Creative Commons Public Domain, for
// any use, without warranty, 2026 by Antmicro.
// SPDX-FileCopyrightText: 2026 Antmicro
// SPDX-License-Identifier: CC0-1.0

// verilog_format: off
`define stop $stop
`define checkd(gotv,expv) do if ((gotv) !== (expv)) begin $write("%%Error: %s:%0d:  got=%0d exp=%0d\n", `__FILE__,`__LINE__, (gotv), (expv)); `stop; end while(0);
// verilog_format: on

module t (
    input clk
);
  // Non-context import, cannot call exports
  import "DPI-C" function int dpii_add(int a, int b);

  // Export that needs a process, never called
  export "DPI-C" function dpix_self;
  function void dpix_self();
    automatic process p = process::self();
  endfunction

  int cyc = 0;
  int sum = 0;
  // Must not become suspendable
  always_ff @(posedge clk) begin
    cyc <= cyc + 1;
    sum <= dpii_add(sum, cyc);
    if (cyc == 10) begin
      `checkd(sum, 45);
      $write("*-* All Finished *-*\n");
      $finish;
    end
  end
endmodule
