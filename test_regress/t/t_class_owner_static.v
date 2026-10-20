// DESCRIPTION: Verilator: Static state follows the object's declaring owner
//
// This file ONLY is placed under the Creative Commons Public Domain.
// SPDX-FileCopyrightText: 2026 Wilson Snyder
// SPDX-License-Identifier: CC0-1.0

// verilog_format: off
`define stop $stop
`define checkd(gotv,expv) do if ((gotv) !== (expv)) begin $write("%%Error: %s:%0d: %m got=%0d exp=%0d\n", `__FILE__,`__LINE__, (gotv), (expv)); `stop; end while(0)
// verilog_format: on

module static_owner;
  int initializations;
  int calls;
  int value = 100;
  function int initialize();
    ++initializations;
    return 10;
  endfunction
  class Counter;
    int payload = 7;
    static int shared = initialize();
    static bit enabled = 1;
    function int local_count();
      static int count = 20;
      return ++count;
    endfunction
    static function int bump();
      return ++shared;
    endfunction
    static function int read_value();
      return shared + value;
    endfunction
    function Counter get();
      ++calls;
      return this;
    endfunction
    static function Counter make();
      Counter result = new;
      return result;
    endfunction
  endclass
  Counter a = Counter::make(), b = new;
endmodule

module t;
  static_owner first (), second ();
  initial begin
    #1;
    `checkd(first.initializations, 1);
    `checkd(second.initializations, 1);
    `checkd($sformatf("%p", first.a), "'{payload:'h7}");
    `checkd(first.a.enabled, 1);
    `checkd(first.b.enabled, 1);
    first.a.enabled = 0;
    `checkd(first.a.enabled, 0);
    `checkd(first.b.enabled, 0);
    `checkd(second.a.enabled, 1);
    first.b.enabled = 1;
    `checkd(first.a.enabled, 1);
    `checkd(first.a.local_count(), 21);
    `checkd(first.b.local_count(), 22);
    `checkd(second.a.local_count(), 21);
    `checkd(first.a.bump(), 11);
    `checkd(first.b.bump(), 12);
    `checkd(second.a.bump(), 11);
    `checkd(first.a.get().read_value(), 112);
    `checkd(first.calls, 1);
    first.a = new;
    `checkd(first.a.bump(), 13);
    `checkd(first.a.local_count(), 23);
    `checkd(first.initializations, 1);
    `checkd(second.initializations, 1);
    $write("*-* All Finished *-*\n");
    $finish;
  end
endmodule
