// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain.
// SPDX-FileCopyrightText: 2026 Antmicro
// SPDX-License-Identifier: CC0-1.0

interface bus_if (
    input logic clk,
    input logic [7:0] in
);
  if (1) begin : g
    logic [7:0] xr = 0;
    logic [7:0] y;
    logic [7:0] w = 0;
    logic [7:0] z;
    always @(posedge clk) xr <= in;
    assign y = xr + 8'd1;
    assign z = w * 8'd2;
  end
  // Called through a virtual interface
  function automatic logic [7:0] get();
    return g.y;
  endfunction
  task automatic set(logic [7:0] v);
    g.w = v;
  endtask
endinterface

module t;
  logic clk = 0;
  always #5 clk = ~clk;
  logic [7:0] a = 3, b = 10;
  bus_if
      b1 (
          clk,
          a
      ),
      b2 (
          clk,
          b
      );
  virtual bus_if v;
  initial begin
    repeat (2) @(posedge clk);
    #1;
    // Only b2.g.y is read other than through v
    if (b2.g.y != 11) $stop;
    v = b1;
    if (v.get() != 4) $stop;
    v.set(5);
    #1;
    if (b1.g.z != 10) $stop;
    if (b2.g.z != 0) $stop;
    $write("*-* All Finished *-*\n");
    $finish;
  end
endmodule
