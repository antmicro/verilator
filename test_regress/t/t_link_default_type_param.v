// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain.
// SPDX-FileCopyrightText: 2026 Antmicro
// SPDX-License-Identifier: CC0-1.0

class class_a;
  typedef logic [31:0] type_1;
endclass

class class_b;
  typedef logic [31:0] type_1;
  typedef logic [63:0] type_2;
endclass

module top #(
    parameter type class_t = class_a
) (
    input logic in_val
);

  typedef class_t::type_2 local_type_2;
  local_type_2 internal_sig;

endmodule

module t;

  top #(.class_t(class_b)) dut (.in_val('0));

  initial begin
    $write("*-* All Finished *-*\n");
    $finish;
  end

endmodule
