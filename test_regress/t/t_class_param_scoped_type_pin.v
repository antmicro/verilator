// DESCRIPTION: Verilator: SystemVerilog regression test
// SPDX-FileCopyrightText: 2026 The Verilator Authors
// SPDX-License-Identifier: CC0-1.0

class packet #(type vc_t = bit);
endclass
class inherited_packet extends packet #(bit [5:0]);
endclass

class holder #(type key_t = int);
  key_t key;
endclass

class sequencer #(type packet_t = packet);
  holder #(packet_t::vc_t) credits = new;
endclass

module t;
  initial begin
    automatic sequencer #(packet #(bit [3:0])) narrow = new;
    automatic sequencer #(packet #(bit [8:0])) wide = new;
    automatic sequencer #(inherited_packet) inherited = new;
    if ($bits(narrow.credits.key) != 4 || $bits(wide.credits.key) != 9) $fatal;
    if ($bits(inherited.credits.key) != 6) $fatal;
    $display("*-* All Finished *-*");
    $finish;
  end
endmodule
