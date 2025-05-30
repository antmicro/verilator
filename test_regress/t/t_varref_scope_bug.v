// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain, for
// any use, without warranty, 2025 by Antmicro.
// SPDX-License-Identifier: CC0-1.0

module t;
 bit read_data;
  class sfr_seq_item;
    bit read_data;
  endclass
  class sfr_monitor_concrete;
    task monitor(sfr_seq_item item);
      item.read_data = read_data;
    endtask
  endclass
endmodule

