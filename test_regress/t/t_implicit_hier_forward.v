// DESCRIPTION: Verilator: Read a hierarchically named net before its implicit declaration
//
// This file ONLY is placed under the Creative Commons Public Domain.
// SPDX-FileCopyrightText: 2026 The Verilator Authors
// SPDX-License-Identifier: CC0-1.0

// verilog_format: off
`define stop $stop
`define checkh(gotv,expv) do if ((gotv) !== (expv)) begin $write("%%Error: %s:%0d:  got=%0x exp=%0x (%s !== %s)\n", `__FILE__,`__LINE__, (gotv), (expv), `"gotv`", `"expv`"); `stop; end while(0);
// verilog_format: on

module t (
    input clk
);
  bit drive;
  int cycle;

  class Reader;
    function logic [3:0] sample;
      return {t.sensed, left.branch.local_net, right.branch.local_net, left.local_net};
    endfunction
    function void override_sensed;
      force t.sensed = 1'b1;
      `checkh(t.sensed, 1'b1);
      release t.sensed;
    endfunction
  endclass

  // IEEE 1800-2023 6.5 and 6.10 permit this hierarchical reference before
  // the continuous assignment implicitly declares the target wire.
  // verilator lint_off IMPLICIT
  assign sensed = drive;
  // verilator lint_on IMPLICIT

  Source left (drive);
  Source right (~drive);

  Reader reader;
  initial reader = new;

  always @(posedge clk) begin
    `checkh(reader.sample(), {drive, drive, ~drive, ~drive});
    `checkh({left.observed, left.shadow}, {drive, ~drive});
    reader.override_sensed();
    if (cycle == 2) begin
      $write("*-* All Finished *-*\n");
      $finish;
    end
    drive <= ~drive;
    cycle <= cycle + 1;
  end
endmodule

module Source (
    input logic drive
);
  wire observed;
  if (1) begin : branch
    // Each module instance has its own generate-local implicit net.
    // verilator lint_off IMPLICIT
    assign local_net = drive;
    assign shadow = drive;
    // verilator lint_on IMPLICIT
    assign observed = shadow;
  end

  // This later declaration creates an independent module-level net of the same name.
  // verilator lint_off IMPLICIT
  assign local_net = ~drive;
  // verilator lint_on IMPLICIT
  // An explicit outer declaration also leaves the earlier local assignment and read in branch.
  wire shadow = ~drive;
endmodule
