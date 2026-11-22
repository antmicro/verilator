// DESCRIPTION: Check the interface owner used by a virtual method before and after the base call.
//
// This file ONLY is placed under the Creative Commons Public Domain.
// SPDX-FileCopyrightText: 2026 The Verilator Authors
// SPDX-License-Identifier: CC0-1.0


class Base;
  int seen;
  function new(int value);
    seen = value;
  endfunction
endclass

interface bank;
  class Reader extends Base;
    static int count;
    virtual function int read();
      return count;
    endfunction
    function new();
      int saved = read();
      super.new(saved);
    endfunction
  endclass
  function automatic void set_count(int value);
    Reader::count = value;
  endfunction
  function automatic int sample ();
    Reader obj = new;
    if (obj.read() != obj.seen) $stop;
    return obj.seen;
  endfunction
endinterface

module t;
  bank left ();
  bank right ();
  virtual bank view;
  initial begin
    left.set_count(11);
    right.set_count(37);
    if (left.sample() != 11 || right.sample() != 37) $stop;
    view = right;
    view.set_count(51);
    if (view.sample() != 51 || left.sample() != 11) $stop;
    $write("*-* All Finished *-*\n");
    $finish;
  end
endmodule
