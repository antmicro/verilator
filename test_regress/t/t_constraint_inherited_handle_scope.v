// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain.
// SPDX-FileCopyrightText: 2026 Wilson Snyder
// SPDX-License-Identifier: CC0-1.0

class payload;
  rand bit items[$];
  constraint one {items.size() == 1; items[0] == 1;}
endclass

class base;
  rand payload p = new;
  function void fill;
    if (p.randomize() != 1) $fatal;
  endfunction
endclass

class derived extends base;
  function void check;
    bit picked;
    if ((std::randomize(picked) with {picked == 1;}) != 1) $fatal;
    if (!picked || p.items.size() != 1 || p.items[0] != 1) $fatal;
  endfunction
endclass

module t;
  initial begin
    automatic derived d = new;
    d.fill();
    d.check();
    $display("*-* All Finished *-*");
    $finish;
  end
endmodule
