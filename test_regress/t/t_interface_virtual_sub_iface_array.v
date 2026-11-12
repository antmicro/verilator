// DESCRIPTION: Verilator: Select an interface instance array through a virtual interface
//
// This file ONLY is placed under the Creative Commons Public Domain.
// SPDX-FileCopyrightText: 2026 The Verilator Authors
// SPDX-License-Identifier: CC0-1.0

// Selecting an element of a nested interface instance array through a virtual
// interface must follow both the declared array indices and the current handle.
interface leaf_if;
  int value;
endinterface

interface child_if;
  leaf_if leaf ();
endinterface

interface parent_if;
  child_if children[2:1] ();
endinterface

module t;
  parent_if a ();
  parent_if b ();
  virtual parent_if vif;
  virtual child_if child;

  initial begin
    a.children[1].leaf.value = 11;
    a.children[2].leaf.value = 12;
    b.children[2].leaf.value = 22;
    vif = a;
    if (vif.children[1].leaf.value !== 11) $stop;
    child = vif.children[2];
    if (child.leaf.value !== 12) $stop;
    vif = b;
    if (vif.children[2].leaf.value !== 22) $stop;
    if (child.leaf.value !== 12) $stop;

    $write("*-* All Finished *-*\n");
    $finish;
  end
endmodule
