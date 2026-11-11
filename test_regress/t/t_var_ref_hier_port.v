// DESCRIPTION: Verilator: Alias hierarchical variables connected to module ref ports
//
// This file ONLY is placed under the Creative Commons Public Domain.
// SPDX-FileCopyrightText: 2026 The Verilator Authors
// SPDX-License-Identifier: CC0-1.0

// Both paths must see writes from either module, including writes to an
// unpacked array made after a task has initialized it through the ref port.
module t (
    input clk
);
  logic write_enable = 0;
  owner source (.*);
  access_port view (source.value, source.words);

  initial begin
    @(negedge clk) view.put();
    @(negedge clk) begin
      if (source.value != 1 || source.words[0] != 2 || source.words[1] != 3) $stop;
      write_enable = 1;
    end
    @(negedge clk) begin
      write_enable = 0;
      if (source.value != 4 || source.words[0] != 2 || source.words[1] != 5) $stop;
      if (view.value != 4 || view.words[0] != 2 || view.words[1] != 5) $stop;
      $write("*-* All Finished *-*\n");
      $finish;
    end
  end
endmodule

module owner (
    input clk,
    input write_enable
);
  int value;
  int words [2];
  always @(posedge clk) begin
    if (write_enable) begin
      value = 4;
      words[1] = 5;
    end
  end
endmodule

module access_port (
    ref int value,
    ref int words [2]
);
  task automatic put();
    value = 1;
    words[0] = 2;
    words[1] = 3;
  endtask
endmodule
