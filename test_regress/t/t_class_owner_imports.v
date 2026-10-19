// DESCRIPTION: Verilator: Owner provenance through interface ports and typedefs
//
// This file ONLY is placed under the Creative Commons Public Domain.
// SPDX-FileCopyrightText: 2026 Wilson Snyder
// SPDX-License-Identifier: CC0-1.0

// Check that interface ports and typedefs preserve the imported class's owner.
// Exercise live owner reads, shared statics and independent object fields.

// verilog_format: off
`define stop $stop
`define checkd(gotv,expv) do if ((gotv) !== (expv)) begin $write("%%Error: %s:%0d: %m got=%0d exp=%0d\n", `__FILE__,`__LINE__, (gotv), (expv)); `stop; end while(0)
// verilog_format: on

interface storage_child_if;
  int y;
endinterface

interface storage_if;
  int x;
  storage_child_if child ();
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

module alias_consumer (
    interface p
);
  // Follow both the generic port binding and the owner's typedef to construct C.
  typedef p.class_t C_t;
  C_t c = new;
  function int through_port();
    return p.c.get();
  endfunction
endmodule

module direct_consumer (
    interface p
);
  // Import the class directly, without the intermediate owner-side alias.
  typedef p.C C_t;
  C_t c = new;
endmodule

module modport_alias_consumer (
    storage_if.r p
);
  // Modport qualification must preserve the same owner provenance as a generic port.
  typedef p.class_t C_t;
  C_t c = new;
  function int through_port();
    return p.c.get();
  endfunction
endmodule

module modport_direct_consumer (
    storage_if.r p
);
  typedef p.C C_t;
  C_t c = new;
endmodule

module context_leaf #(
    parameter int ID = 1
) (
    output bit done
);
  storage_if owner ();
  alias_consumer a (owner), b (owner);
  direct_consumer direct (owner);
  modport_alias_consumer ma (owner.r);
  modport_direct_consumer md (owner.r);

  task automatic check_state(int delta);
    // Every object and consumer of this owner must see the same static storage.
    `checkd(owner.c.get_shared(), ID + 20 + delta);
    `checkd(owner.c2.get_shared(), ID + 20 + delta);
    `checkd(a.c.get_shared(), ID + 20 + delta);
    `checkd(b.c.get_shared(), ID + 20 + delta);
    `checkd(direct.c.get_shared(), ID + 20 + delta);
    `checkd(ma.c.get_shared(), ID + 20 + delta);
    `checkd(md.c.get_shared(), ID + 20 + delta);
    // Imported objects must read the selected owner's live state, not another instance's.
    `checkd(owner.c.get(), ID + 30 + delta);
    `checkd(owner.c2.get(), ID + 30 + delta);
    `checkd(a.c.get(), ID + 30 + delta);
    `checkd(b.c.get(), ID + 30 + delta);
    `checkd(direct.c.get(), ID + 30 + delta);
    `checkd(ma.c.get(), ID + 30 + delta);
    `checkd(md.c.get(), ID + 30 + delta);
    `checkd(a.through_port(), ID + 30 + delta);
    `checkd(ma.through_port(), ID + 30 + delta);
    // The imported owner path must also reach its child interface.
    `checkd(a.c.get_child(), ID + 40 + delta);
    `checkd(b.c.get_child(), ID + 40 + delta);
    `checkd(direct.c.get_child(), ID + 40 + delta);
    // Shared owner context must not merge the objects' individual fields.
    `checkd(owner.c.get_value(), ID + 210 + delta);
    `checkd(owner.c2.get_value(), ID + 220);
    `checkd(a.c.value, ID + 50 + delta);
    `checkd(b.c.value, ID + 60);
  endtask

  initial begin
    owner.x = ID + 30;
    owner.child.y = ID + 40;
    #1;
    owner.c.shared = ID + 20;
    owner.c.value = ID + 210;
    owner.c2.value = ID + 220;
    a.c.value = ID + 50;
    b.c.value = ID + 60;
    #1;
    check_state(0);
    owner.c.value = ID + 310;
    // Write through a modport-imported object; all consumers must observe the update.
    ma.c.shared = ID + 120;
    owner.x = ID + 130;
    owner.child.y = ID + 140;
    a.c.value = ID + 150;
    #1;
    check_state(100);
    done = 1;
  end
endmodule

module context_chain #(
    parameter int ID = 1
) (
    output wire done
);
  context_leaf #(ID) leaf (done);
endmodule

module t;
  wire [2:0] done;
  // Identical owner declarations at different depths must retain separate static state.
  context_leaf #(1) flat_first (done[0]);
  context_leaf #(2) flat_second (done[1]);
  context_chain #(3) linear (done[2]);

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
