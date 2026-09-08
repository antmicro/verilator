// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain.
// SPDX-FileCopyrightText: 2026 Antmicro
// SPDX-License-Identifier: CC0-1.0

class base #(
    type T = int
);
endclass

class missing_type extends base #(int);
  missing_t value;
endclass

interface class interface_base #(
    type T = int
);
endclass

virtual class implemented_type implements interface_base #(int);
  T value;
endclass

class nonclass_base #(
    type B = int
) extends B;
  missing_t value;
endclass

module t;
  missing_type missing;
  implemented_type implemented;
  nonclass_base #() nonclass;
endmodule
