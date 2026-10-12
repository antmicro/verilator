// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain.
// SPDX-FileCopyrightText: 2026 Antmicro
// SPDX-License-Identifier: CC0-1.0

typedef struct {
  logic [3:0] first;
  logic [2:0] second;
} unpacked_struct_t;

module t;
  localparam bit MATCH = eval_case_inside_range_string("c") == 1;
  localparam unpacked_struct_t RESULT = select(1'bx);

  function automatic int eval_case_inside_range_string(input string value);
    case (value) inside
      ["b":"d"]: eval_case_inside_range_string = 1;
      default: eval_case_inside_range_string = 2;
    endcase
  endfunction

  function automatic unpacked_struct_t select(input logic cond);
    return cond ? unpacked_struct_t'{4'b1011, 3'b001}
                : unpacked_struct_t'{4'b1001, 3'b011};
  endfunction
endmodule
