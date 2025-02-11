// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed into the Public Domain, for any use,
// without warranty, 2025 by Antmicro.
// SPDX-License-Identifier: CC0-1.0

`define UVM_MAJOR_REV 1
`define UVM_MINOR_REV 2
`define UVM_NAME UVM
`define UVM_VERSION_STRING `"`UVM_NAME``-```UVM_MAJOR_REV``.```UVM_MINOR_REV```"

module t (/*AUTOARG*/);
   string uvm_revision = `UVM_VERSION_STRING;
   initial begin
      $display(uvm_revision);
      $write("*-* All Finished *-*\n");
      $finish;
   end

endmodule
