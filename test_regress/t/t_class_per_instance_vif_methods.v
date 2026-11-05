// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain.
// SPDX-FileCopyrightText: 2026 Antmicro
// SPDX-License-Identifier: CC0-1.0

interface ifc;
  int x;
  int ctors;
  class Base;
    int b;
    function new(int arg);
      b = arg + x;
      ++ctors;
    endfunction
    function int base_get();
      return x;
    endfunction
    virtual function int over();
      return x * 10;
    endfunction
  endclass
  class Derived extends Base;
    function new(int arg);
      super.new(arg + x);
    endfunction
    function int helper(int k);
      int tmp;
      tmp = x + k;
      return tmp;
    endfunction
    function int calls();
      return helper(100);
    endfunction
    virtual function int over();
      return super.over() + 1;
    endfunction
  endclass
  Derived d;
  Base b;
  function void make();
    b = new(3);
  endfunction
endinterface

module t;
  ifc i1 (), i2 ();
  virtual ifc v;
  initial begin
    i1.x = 1;
    i2.x = 2;
    i1.d = new(5);
    i2.d = new(6);
    i1.make();
    i2.make();
    v = i2;
    #1;
    // b = arg + 2 * x
    if (i1.d.b !== 7) $stop;
    if (v.d.b !== 10) $stop;
    if (i1.ctors !== 2) $stop;
    if (v.ctors !== 2) $stop;
    if (i1.d.base_get() !== 1) $stop;
    if (v.d.base_get() !== 2) $stop;
    if (i1.d.over() !== 11) $stop;
    if (v.d.over() !== 21) $stop;
    if (i1.d.calls() !== 101) $stop;
    if (v.d.calls() !== 102) $stop;
    if (i1.b.b !== 4) $stop;
    if (v.b.over() !== 20) $stop;
    $write("*-* All Finished *-*\n");
    $finish;
  end
endmodule
