// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain.
// SPDX-FileCopyrightText: 2026 Antmicro
// SPDX-License-Identifier: CC0-1.0

module hb (
    input int i,
    output int o
);
  /*verilator hier_block*/
  class C;
    function int get();
      return i;
    endfunction
  endclass
  C c = new;
  always_comb o = c.get();
endmodule

module t;
  int o1, o2;
  hb a (
      10,
      o1
  );
  hb b (
      20,
      o2
  );
  initial begin
    #1;
    if (o1 !== 10) $stop;
    if (o2 !== 20) $stop;
    $write("*-* All Finished *-*\n");
    $finish;
  end
endmodule
