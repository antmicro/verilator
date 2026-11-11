// DESCRIPTION: Randomize an enum used only by a derived constraint, with and without that constraint.
//
// This file ONLY is placed under the Creative Commons Public Domain.
// SPDX-FileCopyrightText: 2026 The Verilator Authors
// SPDX-License-Identifier: CC0-1.0

// verilog_format: off
`define stop $stop
`define checkh(gotv,expv) do if ((gotv) !== (expv)) begin $write("%%Error: %s:%0d:  got=%0x exp=%0x (%s !== %s)\n", `__FILE__,`__LINE__, (gotv), (expv), `"gotv`", `"expv`"); `stop; end while(0);
// verilog_format: on

typedef enum int {
  Red = 2,
  Green = 5,
  Blue = 9
} color_t;

typedef class Palette;

class Limited extends Palette;
  constraint limit_color {color == Green;}
endclass

class Palette;
  rand color_t color;
  rand bit enabled;
  constraint enable {enabled == 1;}
endclass

module t;
  initial begin
    Palette palette;
    Limited limited;
    int success;
    bit base_changed, child_changed;

    palette = new;
    limited = new;
    palette.color = Green;
    // The base has no limit_color constraint. Its enum must vary even though the
    // inline solve only mentions enabled. The derived object's constraint applies.
    repeat (32) begin
`ifdef TEST_INLINE
      success = palette.randomize() with {enabled == 1;};
`else
      success = palette.randomize();
`endif
      `checkh(success, 1);
      `checkh(palette.enabled, 1'b1);
      `checkh((palette.color inside {Red, Green, Blue}), 1'b1);
      base_changed |= palette.color != Green;
      success = limited.randomize();
      `checkh(success, 1);
      `checkh(limited.color, Green);
    end
    `checkh(base_changed, 1'b1);

    // Disabling the sole reference to color must preserve its domain and variation.
    limited.limit_color.constraint_mode(0);
    repeat (32) begin
      success = limited.randomize() with {enabled == 1;};
      `checkh(success, 1);
      `checkh(limited.enabled, 1'b1);
      `checkh((limited.color inside {Red, Green, Blue}), 1'b1);
      child_changed |= limited.color != Green;
    end
    `checkh(child_changed, 1'b1);

`ifdef TEST_INLINE
    // Also allow an explicit choice outside the derived object's restriction.
    success = palette.randomize() with {color == Blue;};
    `checkh(success, 1);
    `checkh(palette.color, Blue);
`endif
    $display("*-* All Finished *-*");
    $finish;
  end
endmodule
