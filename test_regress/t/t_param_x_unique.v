// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain.
// SPDX-FileCopyrightText: 2024 Wilson Snyder
// SPDX-License-Identifier: CC0-1.0

`define stop $stop
`define checkh(gotv, expv) \
  do if ((gotv) !== (expv)) begin \
    $write("%%Error: %s:%0d: got='h%x exp='h%x\n", `__FILE__, `__LINE__, (gotv), (expv)); \
    `stop; \
  end while (0)

module sub #(
    parameter logic [1:0] P = 2'bx0
) (
    output logic is_x,
    output logic is_z
);
  assign is_x = P === 2'bx0;
  assign is_z = P === 2'bz0;
endmodule

module t;
  typedef logic [3:0] array_t[2];
  typedef logic [2:0] nested_array_t[2][2];
  typedef struct packed {
    logic [3:0] first;
    logic [2:0] second;
  } packed_struct_t;
  typedef string string_array_t[2];

  logic default_is_x;
  logic default_is_z;
  logic explicit_is_x;
  logic explicit_is_z;
  logic x_is_x;
  logic x_is_z;
  logic z_is_x;
  logic z_is_z;

  sub sub_default (default_is_x, default_is_z);
  sub #(.P(2'bx0)) sub_explicit (explicit_is_x, explicit_is_z);
  sub #(.P(2'bx0)) sub_x (x_is_x, x_is_z);
  sub #(.P(2'bz0)) sub_z (z_is_x, z_is_z);

  function automatic logic eval_and(input logic lhs, input logic rhs);
    return lhs && rhs;
  endfunction

  function automatic logic eval_or(input logic lhs, input logic rhs);
    return lhs || rhs;
  endfunction

  function automatic logic eval_if(input logic lhs, input logic rhs);
    return lhs -> rhs;
  endfunction

  function automatic logic [6:0] eval_cond(input logic cond, input logic [6:0] lhs,
                                            input logic [6:0] rhs);
    return cond ? lhs : rhs;
  endfunction

  function automatic logic [32:0] eval_cond_wide(input logic cond, input logic [32:0] lhs,
                                                  input logic [32:0] rhs);
    return cond ? lhs : rhs;
  endfunction

  function automatic string eval_string(input logic cond);
    return cond ? "then" : "else";
  endfunction

  function automatic real eval_real(input logic cond);
    return cond ? 1.25 : 2.5;
  endfunction

  function automatic string eval_string_values(input logic cond, input string lhs,
                                                 input string rhs);
    return cond ? lhs : rhs;
  endfunction

  function automatic real eval_real_values(input logic cond, input real lhs, input real rhs);
    return cond ? lhs : rhs;
  endfunction

  function automatic array_t eval_array(input logic cond, input array_t lhs, input array_t rhs);
    return cond ? lhs : rhs;
  endfunction

  function automatic packed_struct_t eval_packed_struct(input logic cond,
                                                         input packed_struct_t lhs,
                                                         input packed_struct_t rhs);
    return cond ? lhs : rhs;
  endfunction

  function automatic bit eval_array_ok(input logic cond, input array_t lhs, input array_t rhs);
    array_t result = eval_array(cond, lhs, rhs);
    return result[0] === 4'b10x1 && result[1] === 4'b0xx1;
  endfunction

  function automatic bit eval_nested_array_ok(input logic cond, input nested_array_t lhs,
                                               input nested_array_t rhs);
    nested_array_t result = cond ? lhs : rhs;
    return result[0][0] === 3'b10x && result[0][1] === 3'b0x1
           && result[1][0] === 3'bx11 && result[1][1] === 3'b1x0;
  endfunction

  function automatic bit eval_string_array_ok(input logic cond, input string_array_t lhs,
                                               input string_array_t rhs);
    string_array_t result = cond ? lhs : rhs;
    return result[0] == "same" && result[1] == "";
  endfunction

  localparam bit AND_X0 = eval_and(1'bx, 1'b0) === 1'b0;
  localparam bit AND_X1 = eval_and(1'bx, 1'b1) === 1'bx;
  localparam bit AND_Z1 = eval_and(1'bz, 1'b1) === 1'bx;
  localparam bit OR_X0 = eval_or(1'bx, 1'b0) === 1'bx;
  localparam bit OR_X1 = eval_or(1'bx, 1'b1) === 1'b1;
  localparam bit OR_Z0 = eval_or(1'bz, 1'b0) === 1'bx;
  localparam bit OR_10 = eval_or(1'b1, 1'b0) === 1'b1;
  localparam bit IF_00 = eval_if(1'b0, 1'b0) === 1'b1;
  localparam bit IF_01 = eval_if(1'b0, 1'b1) === 1'b1;
  localparam bit IF_10 = eval_if(1'b1, 1'b0) === 1'b0;
  localparam bit IF_11 = eval_if(1'b1, 1'b1) === 1'b1;
  localparam bit IF_X0 = eval_if(1'bx, 1'b0) === 1'bx;
  localparam bit IF_X1 = eval_if(1'bx, 1'b1) === 1'b1;
  localparam bit IF_Z0 = eval_if(1'bz, 1'b0) === 1'bx;
  localparam bit COND_X = eval_cond(1'bx, 7'b10xz011, 7'b10xz101) === 7'b10xxxx1;
  localparam bit COND_Z = eval_cond(1'bz, 7'b10xz011, 7'b10xz101) === 7'b10xxxx1;
  localparam bit COND_WIDE
      = eval_cond_wide(1'bx, {1'b0, 31'h1234567, 1'b0},
                       {1'b1, 31'h1234567, 1'b1}) === {1'bx, 31'h1234567, 1'bx};
  localparam bit STRING_TRUE = eval_string(1'b1) == "then";
  localparam bit STRING_FALSE = eval_string(1'b0) == "else";
  localparam bit STRING_X_SAME = eval_string_values(1'bx, "same", "same") == "same";
  localparam bit STRING_X_DIFF = eval_string_values(1'bx, "then", "else") == "";
  localparam bit REAL_TRUE = eval_real(1'b1) == 1.25;
  localparam bit REAL_FALSE = eval_real(1'b0) == 2.5;
  localparam bit REAL_X_SAME = eval_real_values(1'bx, 1.25, 1.25) == 1.25;
  localparam bit REAL_X_DIFF = eval_real_values(1'bx, 1.25, 2.5) == 0.0;
  localparam array_t ARRAY_A = '{4'b1011, 4'b0011};
  localparam array_t ARRAY_B = '{4'b1001, 4'b0101};
  localparam bit ARRAY_X_OK = eval_array_ok(1'bx, ARRAY_A, ARRAY_B);
  localparam bit PACKED_STRUCT_X
      = eval_packed_struct(1'bx, '{4'b1011, 3'b001}, '{4'b1001, 3'b011})
        === packed_struct_t'{4'b10x1, 3'b0x1};
  localparam nested_array_t NESTED_ARRAY_A = '{'{3'b101, 3'b001}, '{3'b011, 3'b110}};
  localparam nested_array_t NESTED_ARRAY_B = '{'{3'b100, 3'b011}, '{3'b111, 3'b100}};
  localparam bit NESTED_ARRAY_X_OK
      = eval_nested_array_ok(1'bx, NESTED_ARRAY_A, NESTED_ARRAY_B);
  localparam string_array_t STRING_ARRAY_A = '{"same", "left"};
  localparam string_array_t STRING_ARRAY_B = '{"same", "right"};
  localparam bit STRING_ARRAY_X_OK
      = eval_string_array_ok(1'bx, STRING_ARRAY_A, STRING_ARRAY_B);

  initial begin
    `checkh({default_is_x, default_is_z}, 2'b10);
    `checkh({explicit_is_x, explicit_is_z}, 2'b10);
    `checkh({x_is_x, x_is_z}, 2'b10);
    `checkh({z_is_x, z_is_z}, 2'b01);
    `checkh({AND_X0, AND_X1, AND_Z1, OR_X0, OR_X1, OR_Z0, OR_10}, 7'b1111111);
    `checkh({IF_00, IF_01, IF_10, IF_11, IF_X0, IF_X1, IF_Z0}, 7'b1111111);
    `checkh({COND_X, COND_Z, COND_WIDE}, 3'b111);
    `checkh({STRING_TRUE, STRING_FALSE, STRING_X_SAME, STRING_X_DIFF}, 4'b1111);
    `checkh({REAL_TRUE, REAL_FALSE, REAL_X_SAME, REAL_X_DIFF}, 4'b1111);
    `checkh({ARRAY_X_OK, PACKED_STRUCT_X, NESTED_ARRAY_X_OK, STRING_ARRAY_X_OK}, 4'b1111);
    $write("*-* All Finished *-*\n");
    $finish;
  end
endmodule
