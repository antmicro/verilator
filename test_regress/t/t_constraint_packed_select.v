// DESCRIPTION: Verilator: Dynamic packed bit and part selects in constraints
//
// This file ONLY is placed under the Creative Commons Public Domain.
// SPDX-FileCopyrightText: 2026 <TODO>
// SPDX-License-Identifier: CC0-1.0

class packet;
  rand int signed idx;
  rand bit [2:0] mask;
  rand bit [13:4] word;
  rand bit [11:0] descending;
  rand bit [74:0] wide;
  rand bit [6:0] widx;
  rand bit enabled;
  int state_lsb;
  constraint c {
    if (enabled) {
      idx inside {[0:2]};
      mask[idx] == 1;
      word[(4+idx) +: 3] == 3'b101;
      descending[(7+idx) -: 3] == 3'b101;
      wide[widx +: 5] == 5'b10110;
      wide[state_lsb +: 5] == 5'b10110;
    } else {
      idx == -1;
      widx == 127;
    }
  }
endclass
module t;
  initial begin
    packet p;
    p = new;
    p.state_lsb = 90;
    if (p.randomize() with {enabled == 0;} != 1) $fatal(1, "inactive");
    p.state_lsb = 68;
    if (p.randomize() with {
          enabled == 1;
          idx == 2;
          mask == 3'b100;
          widx == 68;
          wide == (75'h16 << 68);
        } != 1) $fatal(1, "active");
    if (p.word[(4+p.idx) +: 3] != 5 || p.descending[(7+p.idx) -: 3] != 5 ||
        p.wide[p.widx +: 5] != 5'b10110) $fatal(1, "part select");
    if (p.randomize() with {enabled == 1; idx == 0; mask == 3'b100;} != 0)
      $fatal(1, "impossible selection");
    $display("*-* All Finished *-*");
    $finish;
  end
endmodule
