// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain.
// SPDX-FileCopyrightText: 2026 Antmicro Ltd
// SPDX-License-Identifier: CC0-1.0

module t;
  class killer;
    function new(process victim);
      victim.kill();
    endfunction
  endclass

  initial begin
    automatic process victim = process::self();
    automatic killer killer_instance = new(victim);
  end
endmodule
