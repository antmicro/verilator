// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain.
// SPDX-FileCopyrightText: 2026 PlanV GmbH
// SPDX-License-Identifier: CC0-1.0

interface handle_if;
  chandle c;
endinterface

module t;
  handle_if sif ();
  virtual handle_if vif = sif;

  initial begin
    vif.c = null;
    $finish;
  end
endmodule
