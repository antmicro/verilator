// DESCRIPTION: Verilator: Nested class owner binding and constructor forwarding
//
// This file ONLY is placed under the Creative Commons Public Domain.
// SPDX-FileCopyrightText: 2026 Wilson Snyder
// SPDX-License-Identifier: CC0-1.0

// Check owner binding through nested aliases, construction and replacement.
// Keep lexical captures, static storage and explicit virtual interfaces distinct.

// verilog_format: off
`define stop $stop
`define checkd(gotv,expv) do if ((gotv) !== (expv)) begin $write("%%Error: %s:%0d: %m got=%0d exp=%0d\n", `__FILE__,`__LINE__, (gotv), (expv)); `stop; end while(0)
// verilog_format: on

interface target_if;
  int x;
endinterface

interface nested_if;
  int x;
  class Outer;
    class Inner;
      static int shared;
      int captured;
      virtual target_if vif;
      function new(virtual target_if target = null);
        // Lexical owner state and the explicit interface argument have separate bindings.
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
      // Outer construction must forward the lexical context into both inner objects.
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

module nested_consumer (
    interface p
);
  // An imported nested alias must retain the interface owner selected by p.
  typedef p.class_t C_t;
  C_t c = new;
endmodule

module nested_case (
    output bit done
);
  target_if a (), b ();
  nested_if first (), second ();
  nested_consumer u (first), v (first), w (second);
  initial begin
    first.x = 1;
    second.x = 2;
    a.x = 10;
    b.x = 20;
    // Cross the explicit targets so they cannot be mistaken for lexical owners.
    first.c = new(b);
    first.d = new;
    second.c = new(a);
    second.d = new;
    #1;
    first.c.a.shared = 1;
    second.c.a.shared = 2;
    first.c.derived.offset = 100;
    second.c.derived.offset = 200;
    `checkd(first.c.a.captured, 1);
    `checkd(second.c.a.captured, 2);
    // Inner statics are shared across outer objects within one owner, not across owners.
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
    // Replacement saves current owner state; old objects retain their original snapshot.
    `checkd(first.c.old.captured, 1);
    `checkd(second.c.old.captured, 2);
    `checkd(first.c.a.captured, 11);
    `checkd(second.c.a.captured, 22);
    first.d.a.shared = 111;
    // Rebinding this field must not alter lexical captures or the saved object's target.
    first.c.a.vif = a;
    a.x = 33;
    b.x = 44;
    first.x = 111;
    second.c.derived.offset = 300;
    #1;
    // Both old and replacement objects still read their original owner's live state.
    `checkd(first.c.old.get_owner(), 111);
    `checkd(first.c.a.get_owner(), 111);
    `checkd(second.c.old.get_owner(), 22);
    `checkd(second.c.a.get_owner(), 22);
    `checkd(first.c.a.shared, 111);
    `checkd(first.c.b.shared, 111);
    `checkd(second.c.a.shared, 2);
    `checkd(u.c.get_owner(), 111);
    `checkd(v.c.get_owner(), 111);
    `checkd(w.c.get_owner(), 22);
    `checkd(first.c.h.get(), 211);
    `checkd(second.c.h.get(), 322);
    `checkd(first.c.a.get_target(), 33);
    `checkd(second.c.a.get_target(), 33);
    `checkd(first.c.old.get_target(), 44);
    done = 1;
  end
endmodule

interface construction_if #(
    parameter int X0 = 1
);
  int x = X0;
  class C;
    int captured;
    function new(int delta);
      // Captures must be initialized before constructor reads and writes of owner state.
      captured = get();
      x = captured + delta;
    endfunction
    function int get();
      return x;
    endfunction
  endclass
  C c = new(10);
endinterface

module construction_case #(
    parameter int X0 = 1
) (
    output bit done
);
  construction_if #(X0) owner ();
  initial begin
    #1;
    `checkd(owner.x, X0 + 10);
    `checkd(owner.c.captured, X0);
    `checkd(owner.c.get(), X0 + 10);
    // A later owner update changes live reads without changing the constructor snapshot.
    owner.x = X0 + 100;
    `checkd(owner.c.get(), X0 + 100);
    `checkd(owner.c.captured, X0);
    done = 1;
  end
endmodule

module t;
  wire [2:0] done;
  nested_case nested (done[0]);
  construction_case #(1) construction_first (done[1]);
  construction_case #(2) construction_second (done[2]);

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
