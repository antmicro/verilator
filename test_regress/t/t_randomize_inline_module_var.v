// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain.
// SPDX-FileCopyrightText: 2026 Wilson Snyder
// SPDX-License-Identifier: CC0-1.0

// A class method's inline constraint must read the current variable from its own module instance.
// verilog_format: off
`define stop $stop
`define checkd(gotv,expv) do if ((gotv) !== (expv)) begin $write("%%Error: %s:%0d:  got=%0d exp=%0d\n", `__FILE__, `__LINE__, (gotv), (expv)); `stop; end while(0);
// verilog_format: on

module sub;
  bit [6:0] setting;

  class Item;
    rand bit [6:0] value;
  endclass

  class Caller;
    function void check(bit [6:0] expected);
      Item item = new;
      int success = item.randomize() with {value == setting;};
      `checkd(success, 1);
      `checkd(item.value, expected);
    endfunction
  endclass

  Caller caller = new;
endmodule
module t;
  sub a ();
  sub b ();
  initial begin
    a.setting = 7'h25;
    b.setting = 7'h56;
    a.caller.check(7'h25);
    b.caller.check(7'h56);
    a.setting = 7'h42;
    a.caller.check(7'h42);
    b.caller.check(7'h56);
    $display("*-* All Finished *-*");
    $finish;
  end
endmodule
