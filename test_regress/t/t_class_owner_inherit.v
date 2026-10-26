// DESCRIPTION: Verilator: Independently bound base and derived class captures
//
// This file ONLY is placed under the Creative Commons Public Domain.
// SPDX-FileCopyrightText: 2026 Wilson Snyder
// SPDX-License-Identifier: CC0-1.0

// Check independent owner binding across inheritance, construction and dispatch.
// Exercise typed and generic ports with shared and separate base owners.

// verilog_format: off
`define stop $stop
`define checkd(gotv,expv) do if ((gotv) !== (expv)) begin $write("%%Error: %s:%0d: %m got=%0d exp=%0d\n", `__FILE__,`__LINE__, (gotv), (expv)); `stop; end while(0)
// verilog_format: on

interface typed_base_if;
  int x;
  class Base;
    int value;
    function int get_base();
      return x;
    endfunction
    virtual function int total();
      return x;
    endfunction
  endclass
endinterface

interface typed_derived_if (
    typed_base_if b
);
  typedef b.Base Base_t;
  int x;
  // The base captures b; this class captures the enclosing derived interface.
  class Derived extends Base_t;
    function int get_derived();
      return x;
    endfunction
    virtual function int total();
      // A super call must use the base subobject's independently bound owner.
      return super.total() + x;
    endfunction
  endclass
  Derived c = new;
  Derived old;
  Base_t factory_object;
  Base_t base_handle;
  function automatic Base_t make(int value);
    // Returning a base handle must preserve the new derived object's captures.
    Derived tmp = new;
    tmp.value = value;
    return tmp;
  endfunction
  function void replace(int value);
    // Replacement must leave the saved object's fields and owner binding intact.
    old = c;
    c = new;
    c.value = value;
    factory_object = make(value + 1000);
    base_handle = c;
  endfunction
endinterface

interface typed_leaf_if (
    typed_derived_if m
);
  typedef m.Derived Mid_t;
  int x;
  class Leaf extends Mid_t;
    function int get_leaf();
      return x;
    endfunction
  endclass
  Leaf c = new;
endinterface

module inheritance_case (
    output bit done
);
  typed_base_if b1 (), b2 ();
  // d2 and shared share a base owner but have distinct derived owners.
  typed_derived_if d1 (b1), d2 (b2), shared (b2);
  typed_leaf_if l1 (d1), l2 (d2);

  task automatic check_state(int delta);
    `checkd(l1.c.get_base(), 10 + delta);
    `checkd(l1.c.get_derived(), 100 + delta);
    `checkd(l1.c.get_leaf(), 1000 + delta);
    `checkd(l2.c.get_base(), 20 + delta);
    `checkd(l2.c.get_derived(), 200 + delta);
    `checkd(l2.c.get_leaf(), 2000 + delta);
    `checkd(shared.c.get_base(), 20 + delta);
    `checkd(shared.c.get_derived(), 300);
    `checkd(d1.old.value, 1);
    `checkd(d2.old.value, 2);
    `checkd(d1.old.get_base(), 10 + delta);
    `checkd(d2.old.get_base(), 20 + delta);
    `checkd(d1.old.get_derived(), 100 + delta);
    `checkd(d2.old.get_derived(), 200 + delta);
    `checkd(d1.c.value, 101);
    `checkd(d2.c.value, 102);
    `checkd(d1.c.get_base(), 10 + delta);
    `checkd(d2.c.get_base(), 20 + delta);
    `checkd(d1.c.get_derived(), 100 + delta);
    `checkd(d2.c.get_derived(), 200 + delta);
    `checkd(d1.factory_object.value, 1101);
    `checkd(d2.factory_object.value, 1102);
    `checkd(d1.factory_object.get_base(), 10 + delta);
    `checkd(d2.factory_object.get_base(), 20 + delta);
    // Direct and base-handle dispatch must reach the same override and owners.
    `checkd(d1.c.total(), 110 + 2 * delta);
    `checkd(d1.base_handle.total(), 110 + 2 * delta);
    `checkd(d2.c.total(), 220 + 2 * delta);
    `checkd(d2.base_handle.total(), 220 + 2 * delta);
  endtask

  initial begin
    b1.x = 10;
    b2.x = 20;
    d1.x = 100;
    d2.x = 200;
    shared.x = 300;
    l1.x = 1000;
    l2.x = 2000;
    #1;
    d1.c.value = 1;
    d2.c.value = 2;
    d1.replace(101);
    d2.replace(102);
    check_state(0);
    b1.x = 11;
    b2.x = 21;
    d1.x = 101;
    d2.x = 201;
    l1.x = 1001;
    l2.x = 2001;
    #1;
    check_state(1);
    done = 1;
  end
endmodule

interface generic_mid_if (
    interface b
);
  typedef b.Base Base_t;
  int x;
  class Mid extends Base_t;
    function int get_mid();
      return x;
    endfunction
  endclass
  Mid c = new;
endinterface

interface generic_leaf_if (
    interface m
);
  typedef m.Mid Mid_t;
  int x;
  class Leaf extends Mid_t;
    function int get_leaf();
      return x;
    endfunction
    function int sum();
      return get_base() + get_mid() + get_leaf();
    endfunction
  endclass
  Leaf c = new;
endinterface

module generic_inheritance_case (
    output bit done
);
  typed_base_if b1 (), b2 ();
  // Generic ports must preserve sharing through the third inheritance level.
  generic_mid_if m1 (b1), m2 (b1), independent (b2);
  generic_leaf_if l1 (m1), l2 (m2);
  task automatic check_state(int base_value);
    `checkd(l1.c.get_base(), base_value);
    `checkd(l1.c.get_mid(), 2);
    `checkd(l1.c.get_leaf(), 8);
    `checkd(l1.c.sum(), base_value + 10);
    `checkd(l2.c.get_base(), base_value);
    `checkd(l2.c.get_mid(), 4);
    `checkd(l2.c.get_leaf(), 16);
    `checkd(l2.c.sum(), base_value + 20);
    `checkd(independent.c.get_base(), 32);
    `checkd(independent.c.get_mid(), 64);
  endtask
  initial begin
    b1.x = 1;
    b2.x = 32;
    m1.x = 2;
    m2.x = 4;
    independent.x = 64;
    l1.x = 8;
    l2.x = 16;
    #1;
    check_state(1);
    // Only descendants of b1 should observe this live base-owner update.
    b1.x = 0;
    #1;
    check_state(0);
    done = 1;
  end
endmodule

module inheritance_nested (
    output wire done
);
  inheritance_case inherited (done);
endmodule

module t;
  wire [1:0] done;
  inheritance_nested nested (done[0]);
  generic_inheritance_case generic_inherited (done[1]);

  initial begin
    wait (&done);
    $write("*-* All Finished *-*\n");
    $finish;
  end
  initial begin
    #100;
    $error("Incomplete scenarios: done=%b", done);
    `stop;
  end
endmodule
