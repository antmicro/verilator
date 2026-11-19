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


// verilog_format: off
`define stop $stop
`define checkd(gotv,expv) do if ((gotv) !== (expv)) begin $write("%%Error: %s:%0d:  got=%0d exp=%0d\n", `__FILE__,`__LINE__, (gotv), (expv)); `stop; end while(0);
// verilog_format: on

class SharedTable;
  rand bit [6:0] data[];
  rand bit [6:0] queue_data[$];
  rand bit flag;
  int length;
endclass

class FirstTable extends SharedTable;
  constraint shape { data.size() == length; queue_data.size() == length + 1; }
endclass

class SecondTable extends SharedTable;
  constraint shape { data.size() == length; queue_data.size() == length + 2; }
endclass

class InlineTable extends SharedTable;
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
    begin
      automatic FirstTable first = new;
      automatic SecondTable second = new;
      automatic InlineTable inline_only = new;
      // Siblings constrain the same inherited declarations independently.
      for (int n = 0; n < 4; ++n) begin
        first.length = n;
        second.length = 5 - n;
        `checkd(first.randomize() with { flag == 0; }, 1);
        `checkd(first.data.size(), n);
        `checkd(first.queue_data.size(), n + 1);
        `checkd(second.randomize() with {
          flag == 1;
          foreach (data[i]) data[i] == 7'(i);
          foreach (queue_data[i]) queue_data[i] == 7'(i + 3);
        }, 1);
        `checkd(second.data.size(), 5 - n);
        `checkd(second.queue_data.size(), 7 - n);
        foreach (second.data[i]) `checkd(second.data[i], 7'(i));
        foreach (second.queue_data[i]) `checkd(second.queue_data[i], 7'(i + 3));
        `checkd(inline_only.randomize() with {
          data.size() == local::n;
          queue_data.size() == local::n + 1;
          foreach (data[i]) data[i] == 7'(i + 7);
        }, 1);
        `checkd(inline_only.data.size(), n);
        `checkd(inline_only.queue_data.size(), n + 1);
        foreach (inline_only.data[i]) `checkd(inline_only.data[i], 7'(i + 7));
      end
    end
    $write("*-* All Finished *-*\n");
    $finish;
  end
endmodule
