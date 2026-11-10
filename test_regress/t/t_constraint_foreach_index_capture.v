// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain
// SPDX-FileCopyrightText: 2026 The Verilator Authors
// SPDX-License-Identifier: CC0-1.0

module t;
  int values[2][2][2];

  function automatic void check(int offset);
    if (std::randomize(values) with {
          foreach (values[i, j, k]) values[i][j][k] == offset + 4 * i + 2 * j + k;
        } != 1) $stop;
    foreach (values[i, j, k]) begin
      if (values[i][j][k] != offset + 4 * i + 2 * j + k) $stop;
    end
  endfunction

  initial begin
    check(11);
    check(23);
    $write("*-* All Finished *-*\n");
    $finish;
  end
endmodule
