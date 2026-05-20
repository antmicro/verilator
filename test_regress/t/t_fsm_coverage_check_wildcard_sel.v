// DESCRIPTION: Verilator: Verilog Test module for SystemVerilog 'alias'
//
// Simple bi-directional transitive alias test.
//
// This file ONLY is placed under the Creative Commons Public Domain
// SPDX-FileCopyrightText: 2026 Antmicro
// SPDX-License-Identifier: CC0-1.0

package test_pkg;
    typedef struct {
        logic [7:0] value[*];
    } struct_depth2;
    typedef struct {
        struct_depth2 a; int b;
    } struct_depth1;
    typedef struct {
        struct_depth1 a;
    } struct_depth0;
endpackage
module controller (
    input test_pkg::struct_depth0 test_in,
);
  localparam int unsigned RecoveryMode = 'h3;
  assign recovery_mode = (test_in.a.a.value[0] == RecoveryMode);

  initial begin
    $write("*-* All Finished *-*\n");
    $finish;
  end
endmodule
