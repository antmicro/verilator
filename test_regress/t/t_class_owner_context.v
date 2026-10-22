// DESCRIPTION: Verilator: Class owner context across hierarchy and construction
//
// This file ONLY is placed under the Creative Commons Public Domain.
// SPDX-FileCopyrightText: 2026 Wilson Snyder
// SPDX-License-Identifier: CC0-1.0

// verilog_format: off
`define stop $stop
`define checkd(gotv,expv) do if ((gotv) !== (expv)) begin $write("%%Error: %s:%0d: %m got=%0d exp=%0d\n", `__FILE__,`__LINE__, (gotv), (expv)); `stop; end while(0)
// verilog_format: on

module scope_grandchild;
  int y;
endmodule

module scope_child;
  int y;
  scope_grandchild child();
endmodule

module scope_owner;
  int x;
  scope_child child();
  class LocalReader;
    int value;
    function int get();
      return x;
    endfunction
    function int get_value();
      return value;
    endfunction
  endclass
  class Reader;
    function int get();
      return child.y;
    endfunction
    function int get_grandchild();
      return child.child.y;
    endfunction
  endclass
  class Derived extends Reader;
  endclass
  class Capture;
    int captured;
    function new();
      captured = get();
    endfunction
    function int get();
      return child.y;
    endfunction
  endclass
  Reader direct = new;
  LocalReader local_reader = new;
  Capture captured;
  Derived inherited = new;
endmodule

interface storage_child_if;
  int y;
endinterface

interface storage_if;
  int x;
  storage_child_if child();
  class C;
    static int shared;
    int value;
    function int get();
      return x;
    endfunction
    function int get_shared();
      return shared;
    endfunction
    function int get_value();
      return value;
    endfunction
    function int get_child();
      return child.y;
    endfunction
  endclass
  typedef C class_t;
  C c = new;
  C c2 = new;
  modport r(input x, c);
endinterface

module alias_consumer(interface p);
  typedef p.class_t C_t;
  C_t c = new;
  C_t c2 = new;
  function int through_port();
    return p.c.get();
  endfunction
endmodule

module direct_consumer(interface p);
  typedef p.C C_t;
  C_t c = new;
  C_t c2 = new;
endmodule

module modport_alias_consumer(storage_if.r p);
  typedef p.class_t C_t;
  C_t c = new;
  C_t c2 = new;
  function int through_port();
    return p.c.get();
  endfunction
endmodule

module modport_direct_consumer(storage_if.r p);
  typedef p.C C_t;
  C_t c = new;
  C_t c2 = new;
endmodule

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

interface typed_derived_if(typed_base_if b);
  typedef b.Base Base_t;
  int x;
  class Derived extends Base_t;
    function int get_derived();
      return x;
    endfunction
    virtual function int total();
      return super.total() + x;
    endfunction
  endclass
  Derived c = new;
  Derived old;
  Base_t factory_object;
  Base_t base_handle;
  function automatic Base_t make(int value);
    Derived tmp = new;
    tmp.value = value;
    return tmp;
  endfunction
  function void replace(int value);
    old = c;
    c = new;
    c.value = value;
    factory_object = make(value + 1000);
    base_handle = c;
  endfunction
endinterface

// The same assertions run in flat, linear, and branching module hierarchies.
module context_leaf #(parameter int ID = 1)(output bit done);
  scope_owner scope();
  storage_if owner();
  alias_consumer a(owner), b(owner);
  direct_consumer direct(owner);
  modport_alias_consumer ma(owner.r);
  modport_direct_consumer md(owner.r);
  typed_base_if base_owner();
  typed_derived_if derived_owner(base_owner);

  task automatic check_state(int delta);
    `checkd(scope.local_reader.get(), ID + delta);
    `checkd(scope.local_reader.get_value(), ID + 200 + delta);
    `checkd(scope.direct.get(), ID + delta);
    `checkd(scope.direct.get_grandchild(), ID + 10 + delta);
    `checkd(scope.inherited.get(), ID + delta);
    `checkd(scope.captured.get(), ID + delta);
    `checkd(scope.captured.captured, ID);
    `checkd(owner.c.get_shared(), ID + 20 + delta);
    `checkd(owner.c2.get_shared(), ID + 20 + delta);
    `checkd(a.c.get_shared(), ID + 20 + delta);
    `checkd(a.c2.get_shared(), ID + 20 + delta);
    `checkd(b.c.get_shared(), ID + 20 + delta);
    `checkd(b.c2.get_shared(), ID + 20 + delta);
    `checkd(direct.c.get_shared(), ID + 20 + delta);
    `checkd(direct.c2.get_shared(), ID + 20 + delta);
    `checkd(ma.c.get_shared(), ID + 20 + delta);
    `checkd(ma.c2.get_shared(), ID + 20 + delta);
    `checkd(md.c.get_shared(), ID + 20 + delta);
    `checkd(md.c2.get_shared(), ID + 20 + delta);
    `checkd(owner.c.get(), ID + 30 + delta);
    `checkd(owner.c2.get(), ID + 30 + delta);
    `checkd(a.c.get(), ID + 30 + delta);
    `checkd(a.c2.get(), ID + 30 + delta);
    `checkd(b.c.get(), ID + 30 + delta);
    `checkd(b.c2.get(), ID + 30 + delta);
    `checkd(direct.c.get(), ID + 30 + delta);
    `checkd(direct.c2.get(), ID + 30 + delta);
    `checkd(ma.c.get(), ID + 30 + delta);
    `checkd(ma.c2.get(), ID + 30 + delta);
    `checkd(md.c.get(), ID + 30 + delta);
    `checkd(md.c2.get(), ID + 30 + delta);
    `checkd(a.through_port(), ID + 30 + delta);
    `checkd(ma.through_port(), ID + 30 + delta);
    `checkd(a.c.get_child(), ID + 40 + delta);
    `checkd(b.c.get_child(), ID + 40 + delta);
    `checkd(direct.c.get_child(), ID + 40 + delta);
    `checkd(direct.c2.get_child(), ID + 40 + delta);
    `checkd(owner.c.get_value(), ID + 210 + delta);
    `checkd(owner.c2.get_value(), ID + 220);
    `checkd(a.c.value, ID + 50 + delta);
    `checkd(b.c.value, ID + 60);
    `checkd(derived_owner.c.get_base(), ID + 70 + delta);
    `checkd(derived_owner.c.get_derived(), ID + 80 + delta);
    `checkd(derived_owner.c.value, ID + 90);
  endtask

  initial begin
    scope.x = ID;
    scope.child.y = ID;
    scope.child.child.y = ID + 10;
    scope.captured = new;
    owner.x = ID + 30;
    owner.child.y = ID + 40;
    base_owner.x = ID + 70;
    derived_owner.x = ID + 80;
    #1;
    owner.c.shared = ID + 20;
    scope.local_reader.value = ID + 200;
    owner.c.value = ID + 210;
    owner.c2.value = ID + 220;
    a.c.value = ID + 50;
    b.c.value = ID + 60;
    derived_owner.c.value = ID + 90;
    #1;
    check_state(0);
    scope.x = ID + 100;
    scope.local_reader.value = ID + 300;
    owner.c.value = ID + 310;
    scope.child.y = ID + 100;
    scope.child.child.y = ID + 110;
    ma.c2.shared = ID + 120;
    owner.x = ID + 130;
    owner.child.y = ID + 140;
    a.c.value = ID + 150;
    base_owner.x = ID + 170;
    derived_owner.x = ID + 180;
    #1;
    check_state(100);
    done = 1;
  end
endmodule

module context_chain #(parameter int ID = 1)(output wire done);
  context_leaf #(ID) leaf(done);
endmodule

module context_branch #(parameter int ID = 1)(output wire done);
  wire left_done, right_done;
  context_chain #(2 * ID) left(left_done);
  context_chain #(2 * ID + 1) right(right_done);
  assign done = left_done && right_done;
endmodule

interface typed_leaf_if(typed_derived_if m);
  typedef m.Derived Mid_t;
  int x;
  class Leaf extends Mid_t;
    function int get_leaf();
      return x;
    endfunction
  endclass
  Leaf c = new;
endinterface

module inheritance_case(output bit done);
  typed_base_if b1(), b2();
  typed_derived_if d1(b1), d2(b2), shared(b2);
  typed_leaf_if l1(d1), l2(d2);

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

interface generic_mid_if(interface b);
  typedef b.Base Base_t;
  int x;
  class Mid extends Base_t;
    function int get_mid();
      return x;
    endfunction
  endclass
  Mid c = new;
endinterface

interface generic_leaf_if(interface m);
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

module generic_inheritance_case(output bit done);
  typed_base_if b1(), b2();
  generic_mid_if m1(b1), m2(b1), independent(b2);
  generic_leaf_if l1(m1), l2(m2);
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
    b1.x = 0;
    #1;
    check_state(0);
    done = 1;
  end
endmodule

interface field_base_if;
  class Base;
    int x;
    function int get_base();
      return x;
    endfunction
  endclass
endinterface

interface field_derived_if(field_base_if b);
  typedef b.Base Base_t;
  int x;
  class OwnerReader extends Base_t;
    function int get();
      return x;
    endfunction
    function void set(int value);
      super.x = value;
    endfunction
  endclass
  class Shadow extends Base_t;
    int x;
    function void set(int base_value, int own_value);
      super.x = base_value;
      this.x = own_value;
    endfunction
    function int get();
      return x;
    endfunction
    function int get_super();
      return super.x;
    endfunction
  endclass
  OwnerReader owner = new;
  Shadow shadowed = new;
endinterface

module field_case #(parameter int ID = 1)(output bit done);
  field_base_if b();
  field_derived_if d(b);
  initial begin
    d.x = ID;
    #1;
    d.owner.set(ID + 10);
    d.shadowed.set(ID + 20, ID + 30);
    `checkd(d.owner.get(), ID);
    `checkd(d.owner.x, ID + 10);
    `checkd(d.owner.get_base(), ID + 10);
    `checkd(d.shadowed.get_base(), ID + 20);
    `checkd(d.shadowed.get_super(), ID + 20);
    `checkd(d.shadowed.get(), ID + 30);
    `checkd(d.shadowed.x, ID + 30);
    `checkd(d.x, ID);
    done = 1;
  end
endmodule

interface target_if;
  int x;
endinterface

interface nested_if;
  int x;
  class Outer;
    class Inner;
      static int shared;
      int value;
      int captured;
      virtual target_if vif;
      function new(virtual target_if target = null);
        captured = x;
        vif = target;
      endfunction
      virtual function int get();
        return get_owner();
      endfunction
      function int get_owner();
        return x;
      endfunction
      function int get_target();
        return vif.x;
      endfunction
    endclass
    class Derived extends Inner;
      int offset;
      virtual function int get();
        return super.get() + offset;
      endfunction
    endclass
    typedef Inner inner_t;
    Inner a, b, old, h;
    Derived derived = new;
    function new(virtual target_if target = null);
      a = new(target);
      b = new(target);
      h = derived;
    endfunction
    function void replace();
      old = a;
      a = new(old.vif);
    endfunction
  endclass
  typedef Outer::inner_t class_t;
  Outer c, d;
endinterface

module nested_consumer(interface p);
  typedef p.class_t C_t;
  C_t c = new;
endmodule

module nested_case(output bit done);
  target_if a(), b();
  nested_if first(), second();
  nested_consumer u(first), v(first), w(second);
  initial begin
    first.x = 1;
    second.x = 2;
    a.x = 10;
    b.x = 20;
    first.c = new(b);
    first.d = new;
    second.c = new(a);
    second.d = new;
    #1;
    first.c.a.shared = 1;
    second.c.a.shared = 2;
    first.c.a.value = 10;
    first.c.b.value = 20;
    first.d.a.value = 30;
    second.c.a.value = 40;
    u.c.value = 50;
    v.c.value = 60;
    w.c.value = 70;
    first.c.derived.offset = 100;
    second.c.derived.offset = 200;
    `checkd(first.c.a.captured, 1);
    `checkd(second.c.a.captured, 2);
    `checkd(first.c.b.shared, 1);
    `checkd(first.d.a.shared, 1);
    `checkd(second.c.b.shared, 2);
    `checkd(first.c.derived.get(), 101);
    `checkd(first.c.h.get(), 101);
    `checkd(second.c.derived.get(), 202);
    `checkd(second.c.h.get(), 202);
    `checkd(first.c.a.get_target(), 20);
    `checkd(second.c.a.get_target(), 10);
    `checkd(u.c.get_owner(), 1);
    `checkd(v.c.get_owner(), 1);
    `checkd(w.c.get_owner(), 2);
    first.x = 11;
    second.x = 22;
    first.c.replace();
    second.c.replace();
    `checkd(first.c.old.captured, 1);
    `checkd(second.c.old.captured, 2);
    `checkd(first.c.a.captured, 11);
    `checkd(second.c.a.captured, 22);
    first.d.a.shared = 111;
    first.c.a.value = 100;
    u.c.value = 500;
    first.c.a.vif = a;
    a.x = 33;
    b.x = 44;
    first.x = 111;
    second.c.derived.offset = 300;
    #1;
    `checkd(first.c.old.get_owner(), 111);
    `checkd(first.c.a.get_owner(), 111);
    `checkd(second.c.old.get_owner(), 22);
    `checkd(second.c.a.get_owner(), 22);
    `checkd(first.c.a.shared, 111);
    `checkd(first.c.b.shared, 111);
    `checkd(second.c.a.shared, 2);
    `checkd(first.c.old.value, 10);
    `checkd(first.c.a.value, 100);
    `checkd(first.c.b.value, 20);
    `checkd(first.d.a.value, 30);
    `checkd(second.c.old.value, 40);
    `checkd(u.c.get_owner(), 111);
    `checkd(v.c.get_owner(), 111);
    `checkd(w.c.get_owner(), 22);
    `checkd(u.c.value, 500);
    `checkd(v.c.value, 60);
    `checkd(w.c.value, 70);
    `checkd(first.c.h.get(), 211);
    `checkd(second.c.h.get(), 322);
    `checkd(first.c.a.get_target(), 33);
    `checkd(second.c.a.get_target(), 33);
    `checkd(first.c.old.get_target(), 44);
    done = 1;
  end
endmodule

interface local_inheritance_if;
  class Base;
    int value;
  endclass
  class Derived extends Base;
    function int get();
      return value;
    endfunction
  endclass
  Derived c = new;
endinterface

module local_inheritance_case(output bit done);
  local_inheritance_if first(), second();
  initial begin
    first.c.value = 1;
    second.c.value = 2;
    #1;
    `checkd(first.c.get(), 1);
    `checkd(second.c.get(), 2);
    first.c.value = 11;
    second.c.value = 22;
    #1;
    `checkd(first.c.get(), 11);
    `checkd(second.c.get(), 22);
    done = 1;
  end
endmodule

interface input_if(input int x);
  class Reader;
    function int get();
      return x;
    endfunction
  endclass
  Reader c = new;
endinterface

interface input_owner_if;
  int x;
  input_if child(.x(x));
  function int get();
    return x;
  endfunction
endinterface

module input_case #(parameter int ID = 1)(output bit done);
  input_owner_if owner();
  initial begin
    owner.x = ID;
    #1;
    `checkd(owner.child.c.get(), ID);
    `checkd(owner.get(), ID);
    owner.x = ID + 100;
    #1;
    `checkd(owner.child.c.get(), ID + 100);
    `checkd(owner.get(), ID + 100);
    done = 1;
  end
endmodule

interface construction_if #(parameter int X0 = 1);
  int x = X0;
  class C;
    int captured;
    function new(int delta);
      captured = get();
      x = captured + delta;
    endfunction
    function int get();
      return x;
    endfunction
  endclass
  C c = new(10);
endinterface

module construction_case #(parameter int X0 = 1)(output bit done);
  construction_if #(X0) owner();
  initial begin
    #1;
    `checkd(owner.x, X0 + 10);
    `checkd(owner.c.captured, X0);
    `checkd(owner.c.get(), X0 + 10);
    owner.x = X0 + 100;
    `checkd(owner.c.get(), X0 + 100);
    `checkd(owner.c.captured, X0);
    done = 1;
  end
endmodule

class BoundReader;
  virtual target_if vif;
  function new(virtual target_if target);
    vif = target;
  endfunction
  function int get();
    return vif.x;
  endfunction
endclass

class TypedBoundReader #(type IF_T = virtual target_if);
  IF_T vif;
  function new(IF_T target);
    vif = target;
  endfunction
  function int get();
    return vif.x;
  endfunction
endclass

class ExplicitReader;
  virtual explicit_binding_if vif;
  function int get();
    return vif.x;
  endfunction
endclass

interface explicit_binding_if;
  int x;
  ExplicitReader c = new;
endinterface

module virtual_binding_case(output bit done);
  target_if a(), b();
  BoundReader first, second;
  TypedBoundReader #(virtual target_if) typed_first, typed_second;
  explicit_binding_if owner_first(), owner_second();
  initial begin
    first = new(a);
    second = new(b);
    typed_first = new(a);
    typed_second = new(b);
    a.x = 1;
    b.x = 2;
    #1;
    owner_first.x = 31;
    owner_second.x = 32;
    owner_first.c.vif = owner_first;
    owner_second.c.vif = owner_second;
    `checkd(first.get(), 1);
    `checkd(second.get(), 2);
    `checkd(typed_first.vif.x, 1);
    `checkd(typed_second.vif.x, 2);
    `checkd(typed_first.get(), 1);
    `checkd(typed_second.get(), 2);
    `checkd(owner_first.c.get(), 31);
    `checkd(owner_second.c.get(), 32);
    a.x = 11;
    b.x = 22;
    owner_first.x = 41;
    owner_second.x = 42;
    #1;
    `checkd(first.get(), 11);
    `checkd(second.get(), 22);
    `checkd(typed_first.vif.x, 11);
    `checkd(typed_second.vif.x, 22);
    `checkd(typed_first.get(), 11);
    `checkd(typed_second.get(), 22);
    `checkd(owner_first.c.get(), 41);
    `checkd(owner_second.c.get(), 42);
    done = 1;
  end
endmodule

interface task_owner_if;
  int x;
  int y;
  class Writer;
    task copy();
      y = x;
    endtask
  endclass
  Writer c = new;
endinterface

module task_owner_case(output bit done);
  task_owner_if first(), second();
  initial begin
    first.x = 1;
    second.x = 2;
    #1;
    first.c.copy();
    `checkd(first.y, 1);
    `checkd(second.y, 0);
    second.c.copy();
    `checkd(first.y, 1);
    `checkd(second.y, 2);
    first.x = 11;
    second.x = 22;
    first.c.copy();
    second.c.copy();
    `checkd(first.y, 11);
    `checkd(second.y, 22);
    done = 1;
  end
endmodule

interface timed_task_if(input logic clk);
  logic [7:0] data;
  bit valid;
  class Driver;
    task send_data(logic [7:0] value);
      @(posedge clk);
      data = value;
      valid = 1;
      @(posedge clk);
      data = '0;
      valid = 0;
    endtask
  endclass
  Driver drv;
  initial drv = new;
endinterface

module timed_task_case(output bit done);
  bit clk = 0;
  always #5 clk = ~clk;
  timed_task_if first(clk), second(clk);
  bit [7:0] first_data[$], second_data[$];
  // Sample between drive edges so checks do not depend on active-region ordering.
  always @(negedge clk) begin
    if (first.valid) first_data.push_back(first.data);
    if (second.valid) second_data.push_back(second.data);
  end
  initial begin
    #1;
    while (first.drv == null || second.drv == null) @(posedge clk);
    first.drv.send_data(8'haa);
    repeat (5) @(posedge clk);
    second.drv.send_data(8'hbb);
    #1;
    `checkd(first_data.size(), 1);
    `checkd(second_data.size(), 1);
    `checkd(first_data[0], 8'haa);
    `checkd(second_data[0], 8'hbb);
    `checkd(first.data, 0);
    `checkd(second.data, 0);
    `checkd(first.valid, 0);
    `checkd(second.valid, 0);
    done = 1;
  end
endmodule

module block_counter(input logic clk, input int step, output int result);
  /* verilator no_inline_module */
  int count;
  function automatic int advance(input int value);
    return value + step;
  endfunction
  initial begin : initialize_count
    count = 0;
  end
  always @(posedge clk) begin : update_count
    count <= advance(count);
  end
  always_comb begin : compute_result
    result = count ^ step;
  end
endmodule

module block_case(output bit done);
  logic clk = 0;
  int result1, result2;
  block_counter first(clk, 1, result1), second(clk, 3, result2);
  initial begin
    repeat (2) begin
      #5 clk = 1;
      #5 clk = 0;
    end
    `checkd(first.count, 2);
    `checkd(result1, 3);
    `checkd(second.count, 6);
    `checkd(result2, 5);
    done = 1;
  end
endmodule

module captured_target;
  int value;
  int doubled;
  always_comb doubled = 2 * value;
endmodule

module uncaptured_neighbor(input int value, output int result);
  assign result = value + 3;
endmodule

module capture_pair_owner;
  captured_target left(), right();
  class Reader;
    function void set_values(int l, int r);
      left.value = l;
      right.value = r;
    endfunction
    function int sum();
      return left.doubled + right.doubled;
    endfunction
  endclass
  Reader c = new;
  Reader copied;
endmodule

module capture_pair_case(output bit done);
  capture_pair_owner first(), second();
  int value, result;
  uncaptured_neighbor neighbor(value, result);
  initial begin
    first.c.set_values(1, 10);
    second.c.set_values(2, 20);
    value = 4;
    #1;
    `checkd(first.c.sum(), 22);
    `checkd(second.c.sum(), 44);
    `checkd(result, 7);
    first.copied = new first.c;
    first.c.set_values(3, 30);
    value = 8;
    #1;
    `checkd(first.copied.sum(), 66);
    `checkd(second.c.sum(), 44);
    `checkd(result, 11);
    done = 1;
  end
endmodule

module t;
  wire [19:0] done;
  context_leaf #(1) flat_first(done[0]);
  context_leaf #(2) flat_second(done[1]);
  context_chain #(3) linear(done[2]);
  context_branch #(2) left(done[3]);
  context_branch #(3) right(done[4]);
  inheritance_case inherited(done[5]);
  generic_inheritance_case generic_inherited(done[6]);
  field_case #(1) fields_first(done[7]);
  field_case #(2) fields_second(done[8]);
  nested_case nested(done[9]);
  local_inheritance_case local_inherited(done[10]);
  input_case #(1) input_first(done[11]);
  input_case #(2) input_second(done[12]);
  construction_case #(1) construction_first(done[13]);
  construction_case #(2) construction_second(done[14]);
  virtual_binding_case virtual_bindings(done[15]);
  block_case blocks(done[16]);
  task_owner_case task_owners(done[17]);
  timed_task_case timed_tasks(done[18]);
  capture_pair_case capture_pairs(done[19]);

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
