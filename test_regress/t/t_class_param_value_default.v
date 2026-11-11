// DESCRIPTION: Verilator: SystemVerilog regression test
// SPDX-FileCopyrightText: 2026 The Verilator Authors
// SPDX-License-Identifier: CC0-1.0

interface bus #(int WIDTH = 1);
  bit [WIDTH-1:0] data;
endinterface

class item #(int WIDTH = 1, type T = int);
  int width = WIDTH;
  T data;
endclass

class sequencer #(bit [2:0] N = 1, int WIDTH = int'(N) + 2, type T = int,
                  type ITEM_T = item #(WIDTH, T), type VIF_T = virtual bus #(WIDTH));
  ITEM_T member = new;
  VIF_T vif;
endclass

module t;
  bus #(5) physical();

  initial begin
    automatic sequencer #(.N(3), .T(byte)) five = new;
    automatic sequencer #(.N(3), .WIDTH(7), .T(bit)) seven = new;
    automatic item #(5, byte) same = five.member;
    five.vif = physical;
    five.vif.data = 5'h11;
    same.data = 8'h53;
    if (five.member.width != 5 || five.member.data != 8'h53) $fatal;
    if ($bits(five.vif.data) != 5 || physical.data != 5'h11) $fatal;
    if (seven.member.width != 7 || $bits(seven.member.data) != 1) $fatal;
    $display("*-* All Finished *-*");
    $finish;
  end
endmodule
