// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain.
// SPDX-FileCopyrightText: 2026 Antmicro
// SPDX-License-Identifier: CC0-1.0

module grandchild;
  int y;
endmodule

module child;
  int y;
  int doubled;
  // A class write to y must trigger this logic
  always_comb doubled = 2 * y;
  grandchild gc ();
endmodule

module owner;
  child ch ();
  class Reader;
    int at_new;
    function new();
      at_new = get();  // Reads the instance before the constructor body
    endfunction
    function int get();
      return ch.y;
    endfunction
    function int get_grandchild();
      return ch.gc.y;
    endfunction
    function int get_doubled();
      return ch.doubled;
    endfunction
    function void set(int a);
      ch.y = a;
    endfunction
  endclass
  Reader r;
  Reader copied;
endmodule

module wrap;
  owner o ();
endmodule

module t;
  // Instances of owner at different depths
  owner o1 ();
  wrap w ();
  initial begin
    o1.ch.y = 1;
    w.o.ch.y = 2;
    o1.ch.gc.y = 11;
    w.o.ch.gc.y = 12;
    o1.r = new;
    w.o.r = new;
    #1;
    if (o1.r.at_new != 1) $stop;
    if (w.o.r.at_new != 2) $stop;
    if (o1.r.get() != 1) $stop;
    if (w.o.r.get() != 2) $stop;
    if (o1.r.get_grandchild() != 11) $stop;
    if (w.o.r.get_grandchild() != 12) $stop;
    o1.r.set(3);
    w.o.r.set(4);
    #1;
    if (o1.ch.doubled != 6) $stop;
    if (w.o.r.get_doubled() != 8) $stop;
    if (o1.r.at_new != 1) $stop;
    // A shallow copy refers to the same instance
    o1.copied = new o1.r;
    o1.r.set(5);
    #1;
    if (o1.copied.get_doubled() != 10) $stop;
    if (w.o.r.get() != 4) $stop;
    $write("*-* All Finished *-*\n");
    $finish;
  end
endmodule
