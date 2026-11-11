// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain.
// SPDX-FileCopyrightText: 2026 The Verilator Authors
// SPDX-License-Identifier: CC0-1.0

class instr_t;
  rand bit [4:0] opcode;
endclass

class plan_t;
  rand instr_t instrs[17];
  constraint c {
    foreach (instrs[i]) instrs[i].opcode == 5'(i + 3);
  }
  function new;
    foreach (instrs[i]) instrs[i] = new;
  endfunction
endclass

module t;
  initial begin
    automatic plan_t p = new;
    if (p.randomize() != 1) $stop;
    foreach (p.instrs[i]) begin
      if (p.instrs[i].opcode != 5'(i + 3)) $stop;
    end
    $write("*-* All Finished *-*\n");
    $finish;
  end
endmodule
