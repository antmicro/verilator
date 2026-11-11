// DESCRIPTION: Verilator: SystemVerilog regression test
// SPDX-FileCopyrightText: 2026 The Verilator Authors
// SPDX-License-Identifier: CC0-1.0

interface sample_if #(type DATA_T = bit [4:0]);
  DATA_T data;
endinterface

class holder #(type IF_T = virtual sample_if);
  IF_T vif;
  function int width(); return $bits(vif.data); endfunction
  function int value(); return int'(vif.data); endfunction
endclass

class agent #(type DATA_T = byte, type IF_T = virtual sample_if #(DATA_T))
  extends holder #(IF_T);
endclass

module t;
  sample_if #(bit [8:0]) pins();
  sample_if default_pins();

  initial begin
    automatic agent #(bit [8:0]) assigned_handle = new;
    automatic agent #(bit [3:0]) unassigned_handle = new;
    automatic agent #(bit [4:0]) explicit_default = new;
    pins.data = 9'h100;
    assigned_handle.vif = pins;
    default_pins.data = 5'h1a;
    explicit_default.vif = default_pins;
    if (assigned_handle.width() != 9 || assigned_handle.value() != 256) $fatal;
    if (unassigned_handle.width() != 4) $fatal;
    if (explicit_default.width() != 5 || explicit_default.value() != 26) $fatal;
    $display("*-* All Finished *-*");
    $finish;
  end
endmodule
