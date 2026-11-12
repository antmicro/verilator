// DESCRIPTION: Verilator: Check that each Container specialization passes its element type to Slot
// and that self() returns a handle to the same Container instance.
// SPDX-FileCopyrightText: 2026 The Verilator Authors
// SPDX-License-Identifier: CC0-1.0

// verilog_format: off
`define stop $stop
`define checkd(gotv,expv) do if ((gotv) !== (expv)) begin $write("%%Error: %s:%0d:  got=%0d exp=%0d (%s !== %s)\n", `__FILE__,`__LINE__, (gotv), (expv), `"gotv`", `"expv`"); `stop; end while(0);
// verilog_format: on

class Container #(type Element = int);
  typedef Container #(Element) owner_t;

  class Slot #(type Value = int);
    Value data;
    static function int width();
      return $bits(Value);
    endfunction
  endclass
  Slot #(Element) slot;
  function int slot_width();
    return Slot#(Element)::width();
  endfunction
  function owner_t self();
    return this;
  endfunction
  function new(Element value);
    slot = new;
    slot.data = value;
  endfunction
endclass

module t;
  initial begin
    automatic Container #(bit [6:0]) narrow = new(7'h53);
    automatic Container #(bit [40:0]) wide = new(41'h1_0123456789);
    `checkd(narrow.self() == narrow, 1'b1);
    `checkd(wide.self() == wide, 1'b1);
    `checkd(narrow.slot_width(), 7);
    `checkd(wide.slot_width(), 41);
    `checkd($bits(narrow.slot.data), 7);
    `checkd($bits(wide.slot.data), 41);
    `checkd(narrow.slot.data, 7'h53);
    `checkd(wide.slot.data, 41'h1_0123456789);
    $display("*-* All Finished *-*");
    $finish;
  end
endmodule
