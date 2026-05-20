// DESCRIPTION: Verilator: FSM coverage ignores associative array selection operators
//
// This file ONLY is placed under the Creative Commons Public Domain.
// SPDX-FileCopyrightText: 2026 Wilson Snyder
// SPDX-License-Identifier: CC0-1.0

module t ();
  logic [7:0] a[*];
  localparam int unsigned b = 'h3;
  assign recovery_mode = (a[0] == b);

  initial begin
    $write("*-* All Finished *-*\n");
    $finish;
  end
endmodule
