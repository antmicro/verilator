// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain.
// SPDX-FileCopyrightText: 2026 Antmicro
// SPDX-License-Identifier: CC0-1.0

class Leaf;
  rand byte unsigned tag;
endclass

class Owned;
  rand Leaf leaf = new;
  rand byte unsigned keys[];
  constraint c {
    keys.size() == 1;
    keys[0] == 8'h17;
    leaf.tag == 8'h29;
  }
endclass

module t;
  initial begin
    automatic Owned o = new;
    repeat (10) begin
      if (o.randomize() != 1) $stop;
      if (o.keys.size() != 1) $stop;
      if (o.keys[0] != 8'h17) $stop;
      if (o.leaf.tag != 8'h29) $stop;
    end
    $write("*-* All Finished *-*\n");
    $finish;
  end
endmodule
