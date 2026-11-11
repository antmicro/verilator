// DESCRIPTION: Verilator: Solve inherited dynamic array elements after an inline resize
//
// This file ONLY is placed under the Creative Commons Public Domain.
// SPDX-FileCopyrightText: 2026 The Verilator Authors
// SPDX-License-Identifier: CC0-1.0

class Table;
  rand int length;
  rand bit [63:0] values[];
  constraint entries {
    values.size() == length;
    foreach (values[i]) values[i][31:0] == i;
  }
endclass

class Derived extends Table;
endclass

module t;
  function automatic void check(Derived obj, int wanted);
    int ok;
    ok = obj.randomize() with {length == local:: wanted;};
    if (ok != 1 || obj.values.size() != wanted) $stop;
    foreach (obj.values[i]) begin
      if (obj.values[i][31:0] != i) $stop;
    end
  endfunction

  initial begin
    Derived obj;
    obj = new;
    // Reuse the same inline call as inherited elements are allocated, removed and added.
    check(obj, 3);
    check(obj, 0);
    check(obj, 5);
    $write("*-* All Finished *-*\n");
    $finish;
  end
endmodule
