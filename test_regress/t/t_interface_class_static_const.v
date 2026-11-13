// DESCRIPTION: Verify literal static class properties across bound and direct interfaces.
//
// This file ONLY is placed under the Creative Commons Public Domain.
// SPDX-FileCopyrightText: 2026 The Verilator Authors
// SPDX-License-Identifier: CC0-1.0

package types;
  virtual class Base;
    pure virtual function string label();
  endclass
endpackage

interface slot;
  class Item extends types::Base;
    const static string NAME = "item";
    function string label();
      return NAME;
    endfunction
  endclass

  int value;
  function automatic types::Base make();
    Item item = new;
    return item;
  endfunction
endinterface

module leaf;
endmodule

bind leaf slot bound ();

module t;
  slot direct ();
  leaf a ();
  leaf b ();
  virtual slot view;
  types::Base items[3];

  initial begin
    direct.value = 3;
    a.bound.value = 5;
    b.bound.value = 7;
    view = direct;
    if (view.value != 3) $stop;
    items[0] = direct.make();
    view = a.bound;
    if (view.value != 5) $stop;
    items[1] = view.make();
    view = b.bound;
    if (view.value != 7) $stop;
    items[2] = view.make();
    foreach (items[i])
      if (items[i].label() != "item") $stop;
    $write("*-* All Finished *-*\n");
    $finish;
  end
endmodule
