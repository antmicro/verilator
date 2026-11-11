// DESCRIPTION: Verilator: Honor overrides when solving constraints on nested objects
// SPDX-FileCopyrightText: 2026 The Verilator Authors
// SPDX-License-Identifier: CC0-1.0

class base_item;
  rand int value;
  rand int other;
  constraint pick { value == 3; }
  constraint extra { value < 4; }
  constraint inherited_value { other == 5; }
endclass

class item extends base_item;
  constraint pick { value == 7; }
  constraint extra {}
endclass

class collection;
  rand item slots[2][1];
  rand item single_item;
  constraint outer {
    foreach (slots[i,j]) slots[i][j].value == 7;
    single_item.value == 7;
  }
  function new;
    foreach (slots[i,j]) slots[i][j] = new;
    single_item = new;
  endfunction
endclass

module t;
  initial begin
    automatic collection c = new;
    if (c.randomize() != 1) $stop;
    foreach (c.slots[i,j]) begin
      if (c.slots[i][j].value != 7 || c.slots[i][j].other != 5) $stop;
    end
    if (c.single_item.value != 7 || c.single_item.other != 5) $stop;
    $display("*-* All Finished *-*");
    $finish;
  end
endmodule
