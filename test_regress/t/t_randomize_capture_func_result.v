// DESCRIPTION: Verilator: Capture a function's result variable in inline constraints
// SPDX-FileCopyrightText: 2026 The Verilator Authors
// SPDX-License-Identifier: CC0-1.0
class Base #(type T = bit);
  rand T x;
endclass

class Item extends Base #(bit [7:0]);
endclass

class Driver;
  function Item make_item(input bit [7:0] wanted);
    make_item = new;
    make_item.x = 0;
    if ((make_item.randomize() with {make_item.x == local::wanted;}) != 1) $stop;
  endfunction

  function int copy_value(Item dst, Item src);
    return dst.randomize() with {dst.x == src.x;};
  endfunction

  function bit [7:0] sample(input Item it, input bit [7:0] seed);
    sample = seed + 8'd1;
    if ((it.randomize() with {x == local::sample;}) != 1) $stop;
    if (it.x != sample) $stop;
  endfunction
endclass

module t;
  initial begin
    automatic Driver driver = new;
    automatic Item a = driver.make_item(8'h26);
    automatic Item b = driver.make_item(8'h93);
    if (a == b || a.x != 8'h26 || b.x != 8'h93) $stop;
    if (driver.sample(a, 8'hfe) != 8'hff || a.x != 8'hff || b.x != 8'h93) $stop;
    if (driver.copy_value(a, b) != 1 || a.x != 8'h93 || b.x != 8'h93) $stop;
    $display("*-* All Finished *-*");
    $finish;
  end
endmodule
