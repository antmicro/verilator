// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain.
// SPDX-FileCopyrightText: 2026 Wilson Snyder
// SPDX-License-Identifier: CC0-1.0

// verilog_format: off
`define stop $stop
`define checkd(gotv,expv) do if ((gotv) !== (expv)) begin $write("%%Error: %s:%0d:  got=%0d exp=%0d\n", `__FILE__,`__LINE__, (gotv), (expv)); `stop; end while(0)
// verilog_format: on

interface backdoor_if;
  task automatic invoke();
    increment_count();
  endtask
endinterface

module upward_owner;
  int count;
  backdoor_if backdoor();
  task automatic increment_count();
    count++;
  endtask
endmodule

module mod #(parameter int K = 3);
  localparam int X = K + 10;
  int x;
  class C #(parameter int K = 100);
    int y;
    function int getx();
      return x;
    endfunction
    function int gety();
      return y;
    endfunction
    function int get_owner_constant();
      return X;
    endfunction
    function int get_class_parameter();
      return K;
    endfunction
  endclass
  C c = new;
  C #(.K(200)) c_override = new;
endmodule

interface iface #(parameter int W = 7);
  int x;
  modport mp(input x);
  class C;
    static int constructed;
    int y;
    bit [W-1:0] payload;
    function new();
      constructed++;
    endfunction
    function int get_constructed();
      return constructed;
    endfunction
    function int getx();
      return x;
    endfunction
    function int gety();
      return y;
    endfunction
  endclass
  C c = new;
  class Outer;
    class Inner;
      function int getx();
        return x;
      endfunction
    endclass
    typedef Inner inner_t;
  endclass
  typedef Outer::inner_t inner_t;
endinterface

interface generic_import(interface b);
  typedef b.C C_t;
  typedef C_t Alias_t;
  class Derived extends Alias_t;
    function new(int initial_y = 0);
      y = initial_y;
    endfunction
    function int inherited_y();
      return y;
    endfunction
  endclass
  Derived derived = new;
  Alias_t c;
endinterface

interface generic_derived(interface b);
  typedef b.Derived Derived_t;
  class Next extends Derived_t;
  endclass
  Next c;
endinterface

interface modport_import(iface.mp b);
  typedef b.C C_t;
  C_t c;
endinterface

module imported_consumer(interface b);
  typedef b.C C_t;
  C_t c = new;
  typedef b.inner_t Inner_t;
  Inner_t inner = new;
endmodule

module t;
  int constructor_calls;
  int constructors_before;
  int wide_constructors_before;
  function int next_constructor_value();
    return ++constructor_calls;
  endfunction
  upward_owner upward_first(), upward_second();
  iface #(3) i1 ();
  iface #(65) i2 ();
  generic_import generic_consumer(i1);
  generic_import wide_consumer(i2);
  generic_derived generic_child(generic_consumer);
  modport_import modport_consumer(i1);
  imported_consumer imported_first(i1);
  imported_consumer imported_second(i2);

  mod #(1) m1 ();
  mod #(2) m2 ();
  initial begin
    `checkd(generic_consumer.c == null, 1);
    `checkd(generic_child.c == null, 1);
    `checkd($bits(generic_consumer.derived.payload), 3);
    `checkd($bits(wide_consumer.derived.payload), 65);
    `checkd(modport_consumer.c == null, 1);
    `checkd(m1.c_override.get_owner_constant(), 11);
    `checkd(m2.c_override.get_owner_constant(), 12);
    `checkd(m1.c_override.get_class_parameter(), 200);
    `checkd(m2.c_override.get_class_parameter(), 200);
    upward_first.backdoor.invoke();
    upward_second.backdoor.invoke();
    upward_second.backdoor.invoke();
    `checkd(upward_first.count, 1);
    `checkd(upward_second.count, 2);
    i1.x = 1;
    i2.x = 2;
    i1.c.y = 3;
    i2.c.y = 4;
    generic_consumer.derived.y = 5;
    wide_consumer.derived.y = 6;
    imported_first.c.y = 7;
    imported_second.c.y = 8;

    m1.x = 1;
    m2.x = 2;
    m1.c.y = 3;
    m2.c.y = 4;

    #1;
    `checkd(i1.c.getx(), 1);
    `checkd(i2.c.getx(), 2);
    `checkd(i1.c.gety(), 3);
    `checkd(i2.c.gety(), 4);
    `checkd(generic_consumer.derived.inherited_y(), 5);
    `checkd(wide_consumer.derived.inherited_y(), 6);
    `checkd(imported_first.c.gety(), 7);
    `checkd(imported_second.c.gety(), 8);
    `checkd(imported_first.inner.getx(), 1);
    `checkd(imported_second.inner.getx(), 2);
    constructors_before = imported_first.c.get_constructed();
    wide_constructors_before = imported_second.c.get_constructed();
    generic_consumer.derived = new(next_constructor_value());
    `checkd(constructor_calls, 1);
    `checkd(generic_consumer.derived.inherited_y(), 1);
    generic_consumer.c = generic_consumer.derived;
    generic_consumer.derived = new(next_constructor_value());
    `checkd(constructor_calls, 2);
    `checkd(generic_consumer.c.gety(), 1);
    `checkd(generic_consumer.derived.inherited_y(), 2);
    `checkd(i1.c.get_constructed(), constructors_before + 2);
    `checkd(imported_first.c.get_constructed(), constructors_before + 2);
    `checkd(imported_second.c.get_constructed(), wide_constructors_before);
    if (generic_consumer.derived.randomize() != 1) $stop;

    `checkd(m1.c.getx(), 1);
    `checkd(m2.c.getx(), 2);
    `checkd(m1.c.gety(), 3);
    `checkd(m2.c.gety(), 4);
    $write("*-* All Finished *-*\n");
    $finish;
  end
endmodule
