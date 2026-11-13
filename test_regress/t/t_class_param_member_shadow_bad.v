// DESCRIPTION: Verilator: SystemVerilog regression test
// SPDX-FileCopyrightText: 2026 The Verilator Authors
// SPDX-License-Identifier: CC0-1.0

class holder #(type T = int);
endclass

class item;
  int item[$];
  typedef holder#(item) holder_t;
endclass

module t;
  item::holder_t obj;
endmodule
