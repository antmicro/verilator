// DESCRIPTION: Check hierarchical interface array elements passed to a virtual interface argument.
//
// This file ONLY is placed under the Creative Commons Public Domain.
// SPDX-FileCopyrightText: 2026 The Verilator Authors
// SPDX-License-Identifier: CC0-1.0

// verilog_format: off
`define stop $stop
`define checkh(gotv,expv) do if ((gotv) !== (expv)) begin $write("%%Error: %s:%0d:  got=%0x exp=%0x (%s !== %s)\n", `__FILE__,`__LINE__, (gotv), (expv), `"gotv`", `"expv`"); `stop; end while(0);
// verilog_format: on

interface bus;
  bit [7:0] value;
endinterface

module leaf;
  bus ports[3:2] ();
endmodule

class writer #(
    type T = int
);
  static function void put(T destination, bit [7:0] value);
    destination.value = value;
  endfunction
endclass

module t;
  leaf left ();
  leaf right ();

  initial begin
    // The argument must keep both its instance path and its selected array index.
    writer#(virtual bus)::put(t.left.ports[2], 8'h43);
    writer#(virtual bus)::put(t.right.ports[3], 8'h95);
    `checkh(left.ports[2].value, 8'h43);
    `checkh(left.ports[3].value, 8'h00);
    `checkh(right.ports[2].value, 8'h00);
    `checkh(right.ports[3].value, 8'h95);
    $display("*-* All Finished *-*");
    $finish;
  end
endmodule
