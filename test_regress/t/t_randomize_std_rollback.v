// DESCRIPTION: Verilator: Preserve all scalar scope-randomization arguments after an unsatisfiable solve
//
// This file ONLY is placed under the Creative Commons Public Domain.
// SPDX-FileCopyrightText: 2026 The Verilator Authors
// SPDX-License-Identifier: CC0-1.0

// verilog_format: off
`define stop $stop
`define checkd(gotv,expv) do if ((gotv) !== (expv)) begin $write("%%Error: %s:%0d:  got=%0d exp=%0d\n", `__FILE__,`__LINE__, (gotv), (expv)); `stop; end while(0);
// verilog_format: on

module t;
  initial begin
    int first, second, held, wanted, success;
    bit conflict;
    for (int step = 0; step < 3; ++step) begin
      wanted = 10 + step;
      conflict = step == 1;
      // The middle solve fails after a successful call at the same randomization site.
      success = std::randomize(
          first, second
      ) with {
        first == wanted;
        second == ~first;
        if (conflict) first == second;
      };
      `checkd(success, int'(!conflict));
      // Both arguments retain the last successful solution, then accept the third one.
      if (!conflict) held = wanted;
      `checkd(first, held);
      `checkd(second, ~held);
    end
    $write("*-* All Finished *-*\n");
    $finish;
  end
endmodule
