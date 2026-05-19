// DESCRIPTION: Verilator: Verilog Test module for SystemVerilog 'alias'
//
// Simple bi-directional transitive alias test.
//
// This file ONLY is placed under the Creative Commons Public Domain
// SPDX-FileCopyrightText: 2026 Antmicro
// SPDX-License-Identifier: CC0-1.0

module controller ();
  logic [7:0] value[int][int];
  localparam int unsigned RecoveryMode = 'h3;
  assign recovery_mode = (value[0][0] == RecoveryMode);

  initial begin
    $write("*-* All Finished *-*\n");
    $finish;
  end
endmodule
