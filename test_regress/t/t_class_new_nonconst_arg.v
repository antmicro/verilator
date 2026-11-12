// DESCRIPTION: Verilator: Check that omitted constructor arguments evaluate once on the new
// object, including through a parameterized base class, and that explicit arguments bypass them.
// SPDX-FileCopyrightText: 2026 The Verilator Authors
// SPDX-License-Identifier: CC0-1.0

// verilog_format: off
`define stop $stop
`define checkd(gotv,expv) do if ((gotv) !== (expv)) begin $write("%%Error: %s:%0d:  got=%0d exp=%0d\n", `__FILE__,`__LINE__, (gotv), (expv)); `stop; end while(0);
// verilog_format: on

package p;
  int next_id;
endpackage

class Base #(
    type T = int
);
  static Base #(T) receiver;
  string seen;
  T id;

  function T record_receiver();
    receiver = this;
    return T'(++p::next_id);
  endfunction

  function new(T id = record_receiver(), string label = "default");
    this.id = id;
    seen = label;
  endfunction
endclass

class Derived extends Base #(byte);
  function new();
    super.new();
  endfunction
endclass

module t;
  initial begin
    Base #(byte) first, second;
    Derived derived;
    p::next_id = 5;
    first = new;
    `checkd(first.id, 6)
    `checkd(Base#(byte)::receiver == first, 1'b1)
    `checkd(first.seen == "default", 1'b1)
    second = new(9, "explicit");
    `checkd(second.id, 9)
    `checkd(second.seen == "explicit", 1'b1)
    `checkd(Base#(byte)::receiver == first, 1'b1)
    second = new(.label("partial"));
    `checkd(second.id, 7)
    `checkd(second.seen == "partial", 1'b1)
    `checkd(Base#(byte)::receiver == second, 1'b1)
    derived = new;
    `checkd(derived.id, 8)
    `checkd(Base#(byte)::receiver == derived, 1'b1)
    `checkd(p::next_id, 8)
    Base#(byte)::receiver = null;
    $display("*-* All Finished *-*");
    $finish;
  end
endmodule
