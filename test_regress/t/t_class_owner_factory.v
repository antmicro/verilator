// DESCRIPTION: Verilator: Package registry initialization before object construction
//
// This file ONLY is placed under the Creative Commons Public Domain.
// SPDX-FileCopyrightText: 2026 Wilson Snyder
// SPDX-License-Identifier: CC0-1.0

// verilog_format: off
`define stop $stop
`define checkd(gotv,expv) do if ((gotv) !== (expv)) begin $write("%%Error: %s:%0d: %m got=%0d exp=%0d\n", `__FILE__,`__LINE__, (gotv), (expv)); `stop; end while(0)
// verilog_format: on

package registry_pkg;
  class Registry #(
      type T
  );
    static Registry #(T) me = get();
    static int initialized;
    static function Registry#(T) get();
      if (me == null) begin
        me = new;
        ++initialized;
        `checkd(me, get());
      end
      return me;
    endfunction
    virtual function T create();
      T result = new;
      return result;
    endfunction
  endclass
endpackage

module factory_child;
  int value;
  class ChildItem;
    static int shared = 1;
    function int get();
      return value;
    endfunction
  endclass
endmodule

module factory_unrelated;
  int value;
endmodule

module factory_nested_owner;
  class NestedItem;
    static int shared = 5;
    function int get();
      return shared;
    endfunction
  endclass
  typedef registry_pkg::Registry#(NestedItem) NestedRegistry;
  NestedRegistry registry = NestedRegistry::get();
  function automatic void check();
    NestedItem first_item;
    NestedItem second_item;
    first_item = registry.create();
    second_item = registry.create();
    `checkd(registry.initialized, 1);
    `checkd(first_item.get(), 5);
    first_item.shared = 31;
    `checkd(second_item.get(), 31);
  endfunction
endmodule

module factory_nested;
  factory_nested_owner test ();
endmodule

module t;
  factory_nested nested ();
  factory_child first (), second ();
  factory_unrelated unrelated ();
  int constructions;
  class Base;
    function new();
      `checkd(name(), "item");
    endfunction
    virtual function string name();
      return "base";
    endfunction
  endclass
  class Item extends Base;
    const static string type_name = "item";
    virtual function string name();
      return type_name;
    endfunction
    static int shared = 10;
    function new();
      ++constructions;
    endfunction
    function int read_child(bit which);
      return which ? t.second.value : first.value;
    endfunction
    function int count();
      return constructions;
    endfunction
    function int read_root();
      return $root.t.second.value;
    endfunction
  endclass
  typedef registry_pkg::Registry#(Item) Registry;
  typedef t.Item ItemAlias;
  typedef registry_pkg::Registry#(ItemAlias) ItemAliasRegistry;
  ItemAliasRegistry item_alias_registry = ItemAliasRegistry::get();
  Registry registry = Registry::get();
  Item item;
  typedef first.ChildItem FirstItem;
  typedef second.ChildItem SecondItem;
  typedef t.first.ChildItem FirstAlias;
  class DefaultFactory #(
      type T = FirstItem
  );
    static function T make();
      T result = new;
      return result;
    endfunction
  endclass
  typedef DefaultFactory#(SecondItem) SecondFactory;
  typedef registry_pkg::Registry#(FirstItem) FirstRegistry;
  typedef registry_pkg::Registry#(SecondItem) SecondRegistry;
  typedef registry_pkg::Registry#(FirstAlias) AliasRegistry;
  AliasRegistry alias_registry = AliasRegistry::get();
  FirstRegistry first_registry = FirstRegistry::get();
  SecondRegistry second_registry = SecondRegistry::get();
  FirstItem first_item;
  SecondItem second_item;
  initial begin
    nested.test.check();
    item = registry.create();
    `checkd(item_alias_registry, registry);
    first.value = 11;
    second.value = 29;
    unrelated.value = 7;
    first_item = first_registry.create();
    second_item = second_registry.create();
    `checkd(alias_registry, first_registry);
    `checkd(first_item.get(), 11);
    `checkd(second_item.get(), 29);
    first_item.shared = 23;
    first_item = first_registry.create();
    `checkd(first_item.shared, 23);
    `checkd(second_item.shared, 1);
    `checkd(first_registry.initialized, 1);
    `checkd(second_registry.initialized, 1);
    second_item = SecondFactory::make();
    `checkd(second_item.get(), 29);
    `checkd(item.read_child(0), 11);
    `checkd(item.read_child(1), 29);
    `checkd(item.read_root(), 29);
    `checkd(unrelated.value, 7);
    `checkd(registry.initialized, 1);
    `checkd(item.count(), 1);
    item = registry.create();
    `checkd(item.count(), 2);
    item.shared = 17;
    item = registry.create();
    `checkd(item.shared, 17);
    $write("*-* All Finished *-*\n");
    $finish;
  end
endmodule
