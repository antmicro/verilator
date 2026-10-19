// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain.
// SPDX-FileCopyrightText: 2026 Wilson Snyder
// SPDX-License-Identifier: CC0-1.0

// verilog_format: off
`define stop $stop
`define checkd(gotv,expv) do if ((gotv) !== (expv)) begin $write("%%Error: %s:%0d:  got=%0d exp=%0d\n", `__FILE__,`__LINE__, (gotv), (expv)); `stop; end while(0)
// verilog_format: on

interface iface;
  int x;
  class C;
    function int get();
      return x;
    endfunction
  endclass
  C c = new;
endinterface

module t;
  iface i1 ();
  iface i2 ();
  initial begin
    i1.x = 1;
    i2.x = 2;
    #1;
    `checkd(i1.c.get(), 1);
    `checkd(i2.c.get(), 2);
    $write("*-* All Finished *-*\n");
    $finish;
  end
endmodule
