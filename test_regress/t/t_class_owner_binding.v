// DESCRIPTION: Verilator: Interface input binding versus explicit virtual interfaces
//
// This file ONLY is placed under the Creative Commons Public Domain.
// SPDX-FileCopyrightText: 2026 Wilson Snyder
// SPDX-License-Identifier: CC0-1.0

// Check interface input propagation and explicit virtual-interface bindings.
// Stored interface values must remain independent of lexical owner captures.

// verilog_format: off
`define stop $stop
`define checkd(gotv,expv) do if ((gotv) !== (expv)) begin $write("%%Error: %s:%0d: %m got=%0d exp=%0d\n", `__FILE__,`__LINE__, (gotv), (expv)); `stop; end while(0)
// verilog_format: on

interface target_if;
  int x;
endinterface

interface input_if (
    input int x
);
  class Reader;
    function int get();
      // Capture this child interface; its input follows the enclosing owner's signal.
      return x;
    endfunction
  endclass
  Reader c = new;
endinterface

interface input_owner_if;
  int x;
  input_if child (.x(x));
  function int get();
    // Class-free calls provide a control for ordinary interface owner lookup.
    return x;
  endfunction
endinterface

module input_case #(
    parameter int ID = 1
) (
    output bit done
);
  input_owner_if owner ();
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

class BoundReader;
  // An external class follows its stored interface, without a lexical owner capture.
  virtual target_if vif;
  function new(virtual target_if target);
    vif = target;
  endfunction
  function int get();
    return vif.x;
  endfunction
endclass

class TypedBoundReader #(
    type IF_T = virtual target_if
);
  // The type-parameterized form must preserve the same explicit runtime binding.
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

module virtual_binding_case (
    output bit done
);
  target_if a (), b ();
  BoundReader first, second;
  TypedBoundReader #(virtual target_if) typed_first, typed_second;
  explicit_binding_if owner_first (), owner_second ();
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
    // Interface-held external objects can explicitly bind back to their containing owner.
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
    // Both constructor-bound and field-bound readers must observe later target updates.
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

module concrete_alias_owner;
  target_if concrete ();
endmodule

module concrete_alias_case #(
    parameter int ID = 73
) (
    output bit done
);
  concrete_alias_owner owner ();
  class Reader;
    function virtual target_if get();
      return owner.concrete;
    endfunction
  endclass
  Reader reader = new;
  virtual target_if vif;
  initial begin
    vif = reader.get();
    owner.concrete.x = ID;
    #1;
    `checkd(vif.x, ID);
    done = 1;
  end
endmodule

module t;
  wire [4:0] done;
  input_case #(1) input_first (done[0]);
  input_case #(2) input_second (done[1]);
  virtual_binding_case virtual_bindings (done[2]);

  concrete_alias_case concrete_alias (done[3]);
  concrete_alias_case #(89) second_alias (done[4]);

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
