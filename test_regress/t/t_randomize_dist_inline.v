// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain
// SPDX-FileCopyrightText: 2026 The Verilator Authors
// SPDX-License-Identifier: CC0-1.0

// Check that inline dist weights guide both scope and class randomization.
// Reverse the weights between batches and require the favored value in both.
class Item;
  rand bit value;
endclass

module t;
  initial begin
    Item item;
    bit value;
    int zero_weight, one_weight;
    item = new;
    for (int phase = 0; phase < 2; ++phase) begin
      bit favored;
      int scope_hits, class_hits;
      favored = (phase == 1);
      scope_hits = 0;
      class_hits = 0;
      zero_weight = favored ? 1 : 255;
      one_weight = favored ? 255 : 1;
      repeat (128) begin
        if (std::randomize(value) with {
              value dist {0 := zero_weight, 1 := one_weight};
            } == 0) $stop;
        if (value == favored) ++scope_hits;
        if (item.randomize() with {
              if (favored) value dist {0 := 1, 1 := 255};
              else value dist {0 := 255, 1 := 1};
            } == 0) $stop;
        if (item.value == favored) ++class_hits;
      end
      if (scope_hits < 112 || class_hits < 112) begin
        $display("%%Error: favored=%0d scope hits=%0d class hits=%0d", favored, scope_hits,
                 class_hits);
        $stop;
      end
    end
    // A hard restriction can eliminate the more heavily weighted candidate.
    if (std::randomize(value) with {value dist {0 := 1, 1 := 255}; value == 0;} == 0) $stop;
    if (value != 0) $stop;
    $write("*-* All Finished *-*\n");
    $finish;
  end
endmodule
