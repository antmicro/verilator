// DESCRIPTION: Verilator: Lexical owner captures versus inherited object fields
//
// This file ONLY is placed under the Creative Commons Public Domain.
// SPDX-FileCopyrightText: 2026 Wilson Snyder
// SPDX-License-Identifier: CC0-1.0

// Check that owner captures remain distinct from inherited and shadowing fields.
// Exercise lexical lookup, this/super selection and ordinary local inheritance.

// verilog_format: off
`define stop $stop
`define checkd(gotv,expv) do if ((gotv) !== (expv)) begin $write("%%Error: %s:%0d: %m got=%0d exp=%0d\n", `__FILE__,`__LINE__, (gotv), (expv)); `stop; end while(0)
// verilog_format: on

interface field_base_if;
  class Base;
    int x;
    function int get_base();
      return x;
    endfunction
  endclass
endinterface

interface field_derived_if (
    field_base_if b
);
  typedef b.Base Base_t;
  int x;
  class OwnerReader extends Base_t;
    function int get();
      // This lexical interface read must not become a read of the base object's x.
      return x;
    endfunction
    function void set(int value);
      // An explicit base-field write must leave the interface's x untouched.
      super.x = value;
    endfunction
  endclass
  class Shadow extends Base_t;
    int x;
    function void set(int base_value, int own_value);
      // These two fields belong to separate subobjects, despite sharing a name.
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

module field_case #(
    parameter int ID = 1
) (
    output bit done
);
  field_base_if b ();
  field_derived_if d (b);
  initial begin
    d.x = ID;
    #1;
    d.owner.set(ID + 10);
    d.shadowed.set(ID + 20, ID + 30);
    // Distinct values expose confusion between interface, base and derived storage.
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

interface local_inheritance_if;
  // No external owner dependency: inherited fields must remain ordinary members.
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

module local_inheritance_case (
    output bit done
);
  local_inheritance_if first (), second ();
  initial begin
    first.c.value = 1;
    second.c.value = 2;
    #1;
    `checkd(first.c.get(), 1);
    `checkd(second.c.get(), 2);
    // Later writes must remain visible through each object's inherited getter.
    first.c.value = 11;
    second.c.value = 22;
    #1;
    `checkd(first.c.get(), 11);
    `checkd(second.c.get(), 22);
    done = 1;
  end
endmodule

module t;
  wire [2:0] done;
  field_case #(1) fields_first (done[0]);
  field_case #(2) fields_second (done[1]);
  local_inheritance_case local_inherited (done[2]);

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
