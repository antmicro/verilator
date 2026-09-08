// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain.
// SPDX-FileCopyrightText: 2026 Antmicro
// SPDX-License-Identifier: CC0-1.0

class base #(
    type T = int
);
endclass

class holder #(
    type T = int
);
endclass

class derived extends base #(int);
  holder #(MISSING) value;
endclass

module t;
  derived d;
endmodule
