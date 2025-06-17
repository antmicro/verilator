// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain, for
// any use, without warranty, 2025 by Antmicro.
// SPDX-License-Identifier: CC0-1.0

class seq_item;
  logic read_data = 1'b0;
  logic bar = 1'b0;
  //logic baz = 1'b0;
endclass
class sfr_master_abstract;

virtual task monitor(seq_item item);

endtask

endclass

module top;

  logic t_inst_val;
  logic t_inst2_val;

  t t_inst(.val(t_inst_val), .valid(1'b1));
  t t_inst2(.val(t_inst2_val), .valid(1'b1));

  initial begin
    t_inst_val = 1'b1;
    t_inst2_val = 1'b0;
    #1;
    if (!t_inst.item.read_data) $stop;
    if (t_inst.item_quz.read_data) $stop;
    if (!t_inst.item.bar) $stop;
    // if (!t_inst.item.baz) $stop;
    if (t_inst2.item.read_data) $stop;
    t_inst_val = 1'b0;
    t_inst2_val = 1'b1;
    #1;
    if (t_inst.item.read_data) $stop;
    if (!t_inst.item_quz.read_data) $stop;
    if (!t_inst.item.bar) $stop;
    // if (!t_inst.item.baz) $stop;
    if (!t_inst2.item.read_data) $stop;
    t_inst_val = 1'b1;
    t_inst2_val = 1'b0;
    $write("*-* All Finished *-*\n");
    $finish;
  end
endmodule

module t(input logic val, input logic valid);
  logic bar = 1'b1;
  //logic baz = 1'b0;
  sfr_master_abstract mon_abs = new();
  monitor_concrete mon = new();
  monitor_concrete::Quz mon_quz = new();
  seq_item item = new();
  seq_item item_quz = new();
  class monitor_concrete extends sfr_master_abstract;
    // Unsupported
    //logic local_baz = baz;
    task static monitor(seq_item item);
      if (valid) begin
        item.read_data = val;
      end
      item.bar = bar;
      //item.baz = local_baz;
    endtask
    class Quz;
      task static monitor(seq_item item);
        item.read_data = !val;
      endtask
    endclass
  endclass
  always @(val) begin
    mon.monitor(item);
    mon_abs.monitor(item);
    mon_quz.monitor(item_quz);
  end
endmodule
