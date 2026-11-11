// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain.
// SPDX-FileCopyrightText: 2026 Wilson Snyder
// SPDX-License-Identifier: CC0-1.0

// verilog_format: off
`define stop $stop
`define checkd(gotv,expv) do if ((gotv) !== (expv)) begin $write("%%Error: %s:%0d:  got=%0d exp=%0d\n", `__FILE__,`__LINE__, (gotv), (expv)); `stop; end while(0);
// verilog_format: on

// Factories for same-named classes in different packages must remain distinct,
// while an alias of either class must reuse its specialization and static state.
package factories;
  class maker #(
      type T
  );
    static int count;
    static function T make();
      T obj = new;
      ++count;
      return obj;
    endfunction
  endclass
endpackage

package first_pkg;
  class item #(
      int Value
  );
    int first = Value + 5;
  endclass
endpackage

package second_pkg;
  class item #(
      int Value
  );
    int second = Value + 27;
  endclass
endpackage

module t;
  typedef first_pkg::item#(7) first_item;
  typedef second_pkg::item#(7) second_item;
  typedef factories::maker#(first_item) first_factory;
  typedef factories::maker#(second_item) second_factory;
  typedef factories::maker#(first_pkg::item#(7)) alias_factory;
  initial begin
    first_item a;
    second_item b;
    a = first_factory::make();
    b = second_factory::make();
    `checkd(a.first, 12);
    `checkd(b.second, 34);
    `checkd(alias_factory::count, 1);
    `checkd(second_factory::count, 1);
    $display("*-* All Finished *-*");
    $finish;
  end
endmodule
