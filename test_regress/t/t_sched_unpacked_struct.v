// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain, for
// any use, without warranty, 2026 by Antmicro.
// SPDX-License-Identifier: CC0-1.0

module t;
  typedef struct {
    int x;
  } struct_t;

  logic clk = 0;
  int cyc = 0;
  struct_t array[1];
  int array2[1];
  int res = 1;
  int abcd = 0;

  initial repeat (100) #1 clk ^= 1;

  always @(posedge clk) begin
    cyc <= cyc + 1;
    if (cyc == 1) begin
      array[0].x <= 1;  // works when =
      array2[0] <= 1;  // works when =
      abcd <= 1;
      $display("Assigned 1");
    end
    if (cyc == 2) begin
      array[0].x = 0;
      array2[0] = 0;
      abcd = 0;
      $display("Assigned 0");
    end
    if (cyc == 3) begin
      if (res != 0) $stop;
      $finish;
    end
  end

  always @abcd $display("abcd %0d", abcd);
  always @(array[0].x) $display("array[0].x %0d", array[0].x);
  always @(array2[0]) $display("array2[0] %0d", array2[0]);
  always @(array[0].x) res = array[0].x; // works when @(array[0].x) is replaced with @(abcd)
endmodule
