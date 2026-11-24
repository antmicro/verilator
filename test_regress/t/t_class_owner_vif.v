// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain.
// SPDX-FileCopyrightText: 2026 Antmicro
// SPDX-License-Identifier: CC0-1.0

interface ifc;
  int x;
  // Interface classes and covergroups get no instance reference
  interface class I;
    pure virtual function int get();
  endclass
  class C implements I;
    virtual function int get();
      return x;
    endfunction
  endclass
  C c = new;
  I i = c;
  covergroup cg;
    coverpoint x;
  endgroup
  cg g = new;
endinterface

// Declared outside the interface
class User;
  virtual ifc vif;
  function int get();
    return vif.c.get() + vif.x;
  endfunction
endclass

module t;
  ifc i1 ();
  ifc i2 ();
  virtual ifc v;
  User u1 = new, u2 = new;
  initial begin
    i1.x = 1;
    i2.x = 2;
    u1.vif = i1;
    u2.vif = i2;
    v = i2;
    #1;
    if (i1.c.get() !== 1) $stop;
    if (i2.c.get() !== 2) $stop;
    if (v.c.get() !== 2) $stop;
    if (i1.i.get() !== 1) $stop;
    if (v.i.get() !== 2) $stop;
    if (u1.get() !== 2) $stop;
    if (u2.get() !== 4) $stop;
    $write("*-* All Finished *-*\n");
    $finish;
  end
endmodule
