// DESCRIPTION: Verilator: Verilog Test module
// SPDX-FileCopyrightText: 2026 The Verilator Authors
// SPDX-License-Identifier: CC0-1.0
package pkg;
  function automatic bit [3:0] half(bit [3:0] x);
    bit [3:0] y;
    y = x / 2;
    return y;
  endfunction

  function automatic bit [3:0] calc(bit [3:0] x);
    bit [3:0] a, b;
    a = x + 1;
    b = 2 * a;
    a = 0;
    return b + a;
  endfunction
endpackage

class Item;
  rand bit [3:0] x, y, z;
  bit [3:0] wanted;
  constraint c {x == wanted; y == pkg::half(x);}
endclass

module t;
  Item obj;
  initial begin
    obj = new;
    for (int i = 0; i < 16; ++i) begin
      obj.wanted = 4'(i);
      if ((obj.randomize() with {z == pkg::calc(x);}) != 1) $stop;
      if (obj.x != 4'(i) || obj.y != 4'(i / 2) || obj.z != 4'(2 * (i + 1))) $stop;
    end
    $display("*-* All Finished *-*");
    $finish;
  end
endmodule
