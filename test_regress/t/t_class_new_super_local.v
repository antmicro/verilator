// DESCRIPTION: Verilator: Check constructor-local initializers run once before the base
// constructor when it is called explicitly, and after it when the call is implicit.
// Check that the base constructor's rand_mode setting and the derived field's default survive.
// SPDX-FileCopyrightText: 2026 The Verilator Authors
// SPDX-License-Identifier: CC0-1.0

// verilog_format: off
`define stop $stop
`define checkd(gotv,expv) do if ((gotv) !== (expv)) begin $write("%%Error: %s:%0d:  got=%0d exp=%0d\n", `__FILE__,`__LINE__, (gotv), (expv)); `stop; end while(0);
// verilog_format: on

package p;
  int events;

  function automatic int record(int tag, int value);
    events = events * 10 + tag;
    return value;
  endfunction
endpackage

class Base;
  rand bit base_rand;
  int seen;

  function new(int value = 5);
    seen = p::record(2, value);
    base_rand.rand_mode(0);
  endfunction

  function int plus_one(int value);
    return value + 1;
  endfunction
endclass

class PassLocal extends Base;
  rand bit derived_rand;
  int result;

  function new(int arg);
    int first = p::record(1, arg + 4);
    int second = first * 2;
    super.new(plus_one(second));
    result = p::record(3, second);
  endfunction
endclass

class UseAfter extends Base;
  int result;

  function new(int arg);
    int value = p::record(1, arg + 4);
    super.new(arg);
    result = p::record(3, value);
  endfunction
endclass

class Implicit extends Base;
  int result;

  function new(int arg);
    // Entry to this block follows the implicit base call.
    begin
      int value = p::record(1, arg + 4);
      result = p::record(3, value);
    end
  endfunction
endclass

module t;
  initial begin
    PassLocal a;
    UseAfter b;
    Implicit c;
    a = new(6);
    `checkd(p::events, 123)
    `checkd(a.seen, 21)
    `checkd(a.result, 20)
    `checkd(a.base_rand.rand_mode(), 0)
    `checkd(a.derived_rand.rand_mode(), 1)
    a = new(7);
    `checkd(p::events, 123123)
    `checkd(a.seen, 23)
    `checkd(a.result, 22)
    p::events = 0;
    b = new(8);
    `checkd(p::events, 123)
    `checkd(b.seen, 8)
    `checkd(b.result, 12)
    p::events = 0;
    c = new(9);
    `checkd(p::events, 213)
    `checkd(c.seen, 5)
    `checkd(c.result, 13)
    $write("*-* All Finished *-*\n");
    $finish;
  end
endmodule
