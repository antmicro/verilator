// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain.
// SPDX-FileCopyrightText: 2026 Wilson Snyder
// SPDX-License-Identifier: CC0-1.0

module t;
  class cell_t;
    rand bit [79:0] value;
  endclass
  class box_t;
    rand cell_t child;
    constraint match_value { child.value == 80'hcafe_0123_4567_89ab_f00d; }
  endclass
  initial begin
    box_t box;
    box = new;
    box.child = new;
    box.child.value.rand_mode(0);
    box.child.value.rand_mode(1);
    if (box.randomize() != 1) $fatal;
    if (box.child.value != 80'hcafe_0123_4567_89ab_f00d) $fatal;
    $display("*-* All Finished *-*");
    $finish;
  end
endmodule
