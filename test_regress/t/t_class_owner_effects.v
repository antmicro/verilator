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
    static int values[$];
    static int array_values[];
    function void append(int next_value);
      values.push_back(next_value);
    endfunction
    function void replace_front(int next_value);
      values[0] = next_value;
    endfunction
    function void remove_front();
      void'(values.pop_front());
    endfunction
    function void clear();
      values.delete();
    endfunction
    function void resize(int next_size, int next_value);
      array_values = new[next_size];
      foreach (array_values[i]) array_values[i] = next_value + i;
    endfunction
    function void replace_array_front(int next_value);
      array_values[0] = next_value;
    endfunction
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
  int queue_size, queue_front;
  int array_size, array_front;
  assign queue_size = Writer::values.size();
  assign queue_front = (queue_size != 0) ? Writer::values[0] : -1;
  assign array_size = Writer::array_values.size();
  assign array_front = (array_size != 0) ? Writer::array_values[0] : -1;
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
      first.writer.append(i);
      second.writer.append(3 * i);
      #1;
      `checkd(first.queue_size, 1);
      `checkd(first.queue_front, i);
      `checkd(second.queue_front, 3 * i);
      first.writer.resize(2, i);
      second.writer.resize(3, 3 * i);
      #1;
      `checkd(first.array_size, 2);
      `checkd(first.array_front, i);
      `checkd(second.array_size, 3);
      `checkd(second.array_front, 3 * i);
      first.writer.replace_front(i + 7);
      #1;
      `checkd(first.queue_size, 1);
      `checkd(first.queue_front, i + 7);
      `checkd(second.queue_front, 3 * i);
      `checkd(first.array_front, i);
      first.writer.replace_array_front(i + 7);
      #1;
      `checkd(first.array_size, 2);
      `checkd(first.array_front, i + 7);
      `checkd(second.array_front, 3 * i);
      // Repeated identical writes must converge without a change trigger loop.
      first.writer.replace_front(i + 7);
      first.writer.replace_array_front(i + 7);
      #1;
      `checkd(first.queue_front, i + 7);
      `checkd(first.array_front, i + 7);
      first.writer.remove_front();
      second.writer.clear();
      #1;
      `checkd(first.queue_size, 0);
      `checkd(first.queue_front, -1);
      `checkd(second.queue_size, 0);
      `checkd(second.queue_front, -1);
      first.writer.resize(0, 0);
      #1;
      `checkd(first.array_size, 0);
      `checkd(first.array_front, -1);
    end
    $write("*-* All Finished *-*\n");
    $finish;
  end
endmodule
