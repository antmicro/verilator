// DESCRIPTION: Associate a generic factory singleton and its objects with the interface instance.
//
// This file ONLY is placed under the Creative Commons Public Domain.
// SPDX-FileCopyrightText: 2026 The Verilator Authors
// SPDX-License-Identifier: CC0-1.0
package p;
  typedef class Token;
  virtual class Base;
    pure virtual function Token kind();
  endclass
  virtual class Token;
    pure virtual function Base create();
  endclass
  class Registry #(type T = Base) extends Token;
    // The factory keeps a type-specific object. The object's virtual methods
    // may be called later, after the interface handle has been reassigned.
    typedef Registry#(T) This;
    static This singleton;
    function Base create();
      T result = new;
      return result;
    endfunction
    static function This get();
      if (singleton == null) singleton = new;
      return singleton;
    endfunction
  endclass
endpackage

interface slot;
  class Item extends p::Base;
    function p::Token kind();
      return p::Registry#(Item)::get();
    endfunction
  endclass
  function automatic p::Token factory();
    return p::Registry#(Item)::get();
  endfunction
endinterface

module t;
  slot a ();
  slot b ();
  virtual slot view;
  p::Token first, second;
  p::Base x, y;
  initial begin
    view = a;
    first = view.factory();
    view = b;
    second = view.factory();
    if (first == null || second == null || first == second) $stop;
    #1;
    x = first.create();
    y = second.create();
    if (x == null || y == null || x == y || x.kind() != first || y.kind() != second) $stop;
    if (a.factory() != first || b.factory() != second) $stop;
    $write("*-* All Finished *-*\n");
    $finish;
  end
  // The owner's lifetime extends through every SystemVerilog final block.
  final begin
    if (first != a.factory() || second != b.factory()) $stop;
    if (x.kind() != first || y.kind() != second) $stop;
  end
endmodule
