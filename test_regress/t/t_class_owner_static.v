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
    function Counter get_null();
      ++calls;
      return null;
    endfunction
    static function Counter make();
      Counter result = new;
      return result;
    endfunction
  endclass
  class Derived extends Counter;
  endclass
  Counter a = Counter::make(), b = new;
  Counter empty;
  Counter empty_array[2];
  Derived empty_derived;
  function int read_null();
    Counter c;
    return c.shared + c.read_value();
  endfunction
endmodule

interface null_base_if;
  class Base;
    static int n = 5;
    static function int get();
      return n;
    endfunction
  endclass
endinterface

interface null_derived_if(interface b);
  typedef b.Base Base_t;
  class Derived extends Base_t;
  endclass
  Derived empty;
endinterface

module null_alias_consumer(interface p);
  typedef p.Base First_t;
  typedef First_t Alias_t;
  Alias_t empty;
endmodule

module t;
  static_owner first (), second ();
  null_base_if base_first(), base_second();
  null_derived_if derived_first(base_first), derived_shared(base_first), derived_second(base_second);
  null_alias_consumer alias_first(base_first), alias_second(base_second);
  initial begin
    #1;
    `checkd(derived_first.empty.n, 5);
    `checkd(alias_first.empty.get(), 5);
    derived_first.empty.n = 17;
    `checkd(derived_shared.empty.get(), 17);
    `checkd(alias_first.empty.n, 17);
    `checkd(derived_second.empty.get(), 5);
    `checkd(alias_second.empty.n, 5);
    `checkd(first.initializations, 1);
    `checkd(second.initializations, 1);
    // Static accesses use the class binding even before any handle is assigned.
    `checkd(first.empty.shared, 10);
    `checkd(first.empty_array[0].shared, 10);
    `checkd(first.empty.read_value(), 110);
    `checkd(first.read_null(), 120);
    first.empty.shared = 30;
    `checkd(first.empty_derived.shared, 30);
    `checkd(first.empty_derived.bump(), 31);
    `checkd(first.a.shared, 31);
    `checkd(second.empty.shared, 10);
    `checkd(second.empty_derived.read_value(), 110);
    first.empty_array[1].shared = 10;
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
    `checkd(first.a.get_null().read_value(), 112);
    `checkd(first.calls, 2);
    `checkd(first.a.get_null().shared, 12);
    `checkd(first.calls, 3);
    first.a.get_null().shared = 12;
    `checkd(first.calls, 4);
    first.a = new;
    `checkd(first.a.bump(), 13);
    `checkd(first.a.local_count(), 23);
    `checkd(first.initializations, 1);
    `checkd(second.initializations, 1);
    $write("*-* All Finished *-*\n");
    $finish;
  end
endmodule
