// DESCRIPTION: Verilator: Later formal does not shadow preceding function ranges
// SPDX-FileCopyrightText: 2026 The Verilator Authors
// SPDX-License-Identifier: CC0-1.0

module t;
  localparam int Width = 3;
  function automatic bit [Width:0] select_arg;
    input bit [Width-1:0] value;
    input int Width;
    return (Width == 9) ? {1'b0, value} : '0;
  endfunction
  initial begin
    if (select_arg(3'd7, 9) != 4'd7 || $bits(select_arg(0, 0)) != 4) $fatal;
    $display("*-* All Finished *-*");
    $finish;
  end
endmodule
