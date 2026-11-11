// DESCRIPTION: Verilator: Preserve ordered soft constraints during relaxation
// SPDX-FileCopyrightText: 2026 The Verilator Authors
// SPDX-License-Identifier: CC0-1.0

class Choice;
  rand bit [3:0] value;
  bit [15:0] legal;
  bit [15:0] want[8];
  constraint c {
    legal[value] == 1;
    soft want[0][value] == 1;
    soft want[1][value] == 1;
    soft want[2][value] == 1;
    soft want[3][value] == 1;
    soft want[4][value] == 1;
    soft want[5][value] == 1;
    soft want[6][value] == 1;
    soft want[7][value] == 1;
  }
endclass

module t;
  function automatic void check(Choice c);
    bit [15:0] viable;
    bit [7:0] expected;
    viable = c.legal;
    for (int i = 7; i >= 0; --i) begin
      if ((viable & c.want[i]) != 0) begin
        expected[i] = 1;
        viable &= c.want[i];
      end
    end
    if (0 == c.randomize()) $stop;
    if (!viable[c.value]) $stop;
    foreach (c.want[i])
      if (c.want[i][c.value] != expected[i]) $stop;
  endfunction

  initial begin
    automatic Choice c = new;
    c.want = '{16'hf0ff, 16'h1234, 16'h0ff0, 16'hf00f,
               16'hffff, 16'h00ff, 16'h0000, 16'hff00};
    c.legal = '1;
    repeat (16) begin
      check(c);
      c.legal[c.value] = 0;
    end
    for (int k = 0; k < 16; ++k) begin
      c.legal = (16'h0001 << k) | (16'h0101 >> (k % 4));
      foreach (c.want[i]) c.want[i] = 16'(32'h615dbf3 * (i + 1) * (k + 3));
      check(c);
    end
    c.value.rand_mode(0);
    c.legal = 16'h0001 << c.value;
    check(c);
    $display("*-* All Finished *-*");
    $finish;
  end
endmodule
