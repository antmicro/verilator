// DESCRIPTION: Verilator: Class owner captures without timing support
//
// This file ONLY is placed under the Creative Commons Public Domain.
// SPDX-FileCopyrightText: 2026 Wilson Snyder
// SPDX-License-Identifier: CC0-1.0

// verilog_format: off
`define stop $stop
`define checkd(gotv,expv) do if ((gotv) !== (expv)) begin $write("%%Error: %s:%0d: %m got=%0d exp=%0d\n", `__FILE__,`__LINE__, (gotv), (expv)); `stop; end while(0)
// verilog_format: on

module capture_owner;
  int value;
  class Reader;
    function int get();
      return value;
    endfunction
  endclass
  Reader c = new;
endmodule

module t;
  capture_owner first(), second();
  initial begin
    first.value = 3;
    second.value = 7;
    `checkd(first.c.get(), 3);
    `checkd(second.c.get(), 7);
    first.value = 11;
    `checkd(first.c.get(), 11);
    `checkd(second.c.get(), 7);
    $write("*-* All Finished *-*\n");
    $finish;
  end
endmodule
