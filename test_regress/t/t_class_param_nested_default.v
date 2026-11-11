// DESCRIPTION: Verilator: SystemVerilog regression test
// SPDX-FileCopyrightText: 2026 The Verilator Authors
// SPDX-License-Identifier: CC0-1.0

class base #(type BASE_T = int);
  BASE_T data;
endclass

class sequencer #(type ITEM_T = int) extends base #(ITEM_T);
endclass

class agent #(type ITEM_T = bit [4:0], type SEQ_T = sequencer #(ITEM_T));
  SEQ_T seqr = new;
endclass

class alias_agent #(type T = bit [4:0], type ITEM_T = T, type SEQ_T = sequencer #(ITEM_T));
  SEQ_T seqr = new;
endclass

module t;
  initial begin
    automatic agent #(bit [6:0]) seven = new;
    automatic agent #(bit [8:0]) nine = new;
    automatic agent #(bit [6:0], sequencer #(bit [2:0])) explicit_seq = new;
    automatic agent #(bit [6:0], sequencer #(bit [6:0])) equivalent = seven;
    automatic alias_agent #() with_defaults = new;
    automatic alias_agent #(bit [4:0], bit [4:0], sequencer #(bit [4:0])) all_explicit
        = with_defaults;
    equivalent.seqr.data = 7'h70;
    if ($bits(seven.seqr.data) != 7 || $bits(nine.seqr.data) != 9) $fatal;
    if ($bits(explicit_seq.seqr.data) != 3 || $bits(all_explicit.seqr.data) != 5) $fatal;
    if (seven.seqr.data != 7'h70) $fatal;
    $display("*-* All Finished *-*");
    $finish;
  end
endmodule
