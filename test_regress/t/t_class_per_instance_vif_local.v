// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain.
// SPDX-FileCopyrightText: 2026 Antmicro
// SPDX-License-Identifier: CC0-1.0

package pkg;
  int count;
  function int bump();
    return ++count;
  endfunction
endpackage

interface ifc #(
    parameter int W = 4
);
  import pkg::*;
  int x;
  localparam int K = 3;
  // Reads only locals, parameters and package items
  class Pkt;
    int data;
    function int calc();
      int tmp = data * K;
      void'(pkg::bump());
      void'(bump());
      return tmp + W;
    endfunction
  endclass
  Pkt p = new;
endinterface

module t;
  ifc i1 (), i2 ();
  virtual ifc v;
  initial begin
    v = i2;
    v.p.data = 5;
    if (v.p.calc() != 19) $stop;
    if (i1.p.calc() != 4) $stop;
    if (pkg::count != 4) $stop;
    $write("*-* All Finished *-*\n");
    $finish;
  end
endmodule
