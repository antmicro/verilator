// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain.
// SPDX-FileCopyrightText: 2026 Wilson Snyder
// SPDX-License-Identifier: CC0-1.0

// verilog_format: off
`define stop $stop
`define checkh(gotv,expv) do if ((gotv) !== (expv)) begin $write("%%Error: %s:%0d:  got='h%x exp='h%x\n", `__FILE__,`__LINE__, (gotv), (expv)); `stop; end while(0);
// verilog_format: on

interface Bus;
  int value;
endinterface

class Config;
  virtual Bus links[];
endclass

module t;
  Bus up[4:6] ();
  Bus down[8:7] ();
  initial begin
    automatic Config cfg = new;
    up[4].value = 10;
    up[5].value = 20;
    up[6].value = 30;
    // Copying instance arrays to a dynamic array must resize it and preserve declaration order.
    cfg.links = up;
    `checkh(cfg.links.size(), 3);
    `checkh(cfg.links[0].value, 10);
    `checkh(cfg.links[1].value, 20);
    `checkh(cfg.links[2].value, 30);
    down[8].value = 44;
    down[7].value = 55;
    cfg.links = down;
    `checkh(cfg.links.size(), 2);
    `checkh(cfg.links[0].value, 44);
    `checkh(cfg.links[1].value, 55);
    // Writes through the copied handles must reach the original interface instance.
    cfg.links[1].value = 66;
    `checkh(down[7].value, 66);
    $display("*-* All Finished *-*");
    $finish;
  end
endmodule
