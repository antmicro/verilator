// DESCRIPTION: Verilator: Allow an inline constraint when randomize omits its argument parentheses
// SPDX-FileCopyrightText: 2026 The Verilator Authors
// SPDX-License-Identifier: CC0-1.0

class item;
  rand int value;
endclass

module t;
  initial begin
    automatic item obj = new;
    if (obj.randomize with {value == 23;} != 1) $stop;
    if (obj.value != 23) $stop;

    // The same call is legal as a statement with its return value discarded.
    void'(obj.randomize with {value == 42;});
    if (obj.value != 42) $stop;
    $display("*-* All Finished *-*");
    $finish;
  end
endmodule
