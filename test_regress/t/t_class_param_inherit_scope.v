// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain.
// SPDX-FileCopyrightText: 2026 Antmicro
// SPDX-License-Identifier: CC0-1.0

// verilog_format: off
`define stop $stop
`define checkd(gotv,expv) do if ((gotv) !== (expv)) begin $write("%%Error: %s:%0d: got=%0d exp=%0d\n", `__FILE__, `__LINE__, (gotv), (expv)); `stop; end while (0);
// verilog_format: on

class constants #(
    int N = 1
);
  typedef bit [N-1:0] data_t;
endclass

class value_base #(
    int N = 1
);
  typedef bit [N-1:0] T;
endclass

class type_base #(
    type T = int
);
endclass

// Each inherited T forces lookup before the base parameter pins are visited.
class scoped_value extends value_base #(constants #(7)::N);
  T value;
endclass

class packed_range extends type_base #(bit [constants #(15)::N:1]);
  T value;
endclass

class bits_query extends value_base #($bits(
    constants #(31)::data_t
));
  T value;
endclass

class type_query #(
    int N = 33
) extends type_base #(type (constants #(N)::data_t));
  T value;
endclass

module scope_probe #(
    int N = 1
) (
    output bit [N-1:0] value
);
  assign value = '1;
endmodule

module t;
  bit [14:0] module_value;
  scope_probe #(15) probe (module_value);
  scoped_value scoped;
  packed_range packed_bits;
  bits_query queried_bits;
  type_query queried_type;
  type_query #(65) wide_type;
  initial begin
    scoped = new;
    packed_bits = new;
    queried_bits = new;
    queried_type = new;
    wide_type = new;
    `checkd($bits(scoped.value), 7);
    `checkd($bits(packed_bits.value), 15);
    `checkd($bits(queried_bits.value), 31);
    `checkd($bits(queried_type.value), 33);
    `checkd($bits(wide_type.value), 65);
    `checkd($bits(module_value), 15);
    $write("*-* All Finished *-*\n");
    $finish;
  end
endmodule
