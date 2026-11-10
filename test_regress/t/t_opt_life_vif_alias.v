// DESCRIPTION: Verilator: Preserve writes through a virtual interface alias
//
// This file ONLY is placed under the Creative Commons Public Domain.
// SPDX-FileCopyrightText: 2026 The Verilator Authors
// SPDX-License-Identifier: CC0-1.0

// A write through a virtual interface can change the value last assigned
// through the instance name in the same process.
interface value_if;
  int value;
endinterface

module t;
  value_if inst ();
  virtual value_if vif;

  initial begin
    inst.value = 1;
    vif = inst;
    vif.value = 2;
    if (inst.value !== 2) $stop;
    $write("*-* All Finished *-*\n");
    $finish;
  end
endmodule
