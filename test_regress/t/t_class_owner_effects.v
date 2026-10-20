// DESCRIPTION: Verilator: Captured storage scheduling without virtual interfaces
//
// This file ONLY is placed under the Creative Commons Public Domain.
// SPDX-FileCopyrightText: 2026 Wilson Snyder
// SPDX-License-Identifier: CC0-1.0

// verilog_format: off
`define stop $stop
`define checkd(gotv,expv) do if ((gotv) !== (expv)) begin $write("%%Error: %s:%0d: %m got=%0d exp=%0d\n", `__FILE__,`__LINE__, (gotv), (expv)); `stop; end while(0)
// verilog_format: on

`ifdef UNRELATED_VIF
interface unrelated_iface;
endinterface
`endif

module effect_owner;
  int value;
  int doubled;
  assign doubled = 2 * value;
  class Writer;
    task put(int next_value);
      // verilator no_inline_task
      value = next_value;
      #1;
    endtask
    function int get();
      return value;
    endfunction
  endclass
  Writer writer = new;
endmodule

module t;
`ifdef UNRELATED_VIF
  virtual unrelated_iface unused_vif;
`endif
  effect_owner first (), second ();
  initial begin
    for (int i = 1; i < 8; ++i) begin
      first.writer.put(i);
      `checkd(first.doubled, 2 * i);
      `checkd(second.doubled, 2 * (i - 1) * 3);
      second.writer.put(3 * i);
      `checkd(second.doubled, 6 * i);
      `checkd(first.writer.get(), i);
    end
    $write("*-* All Finished *-*\n");
    $finish;
  end
endmodule
