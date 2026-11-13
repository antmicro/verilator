// DESCRIPTION: Verilator: Check shared writes from independent timing domains
//
// This file ONLY is placed under the Creative Commons Public Domain.
// SPDX-FileCopyrightText: 2026 Wilson Snyder
// SPDX-License-Identifier: CC0-1.0

module t;
  bit clk_a, clk_b;
  wire [95:0] seen_a, seen_b, seen_shared;
  wire all_a = &seen_a;
  wire all_b = &seen_b;

  class Writer;
    static function void update(ref bit value, input bit next_value = 1);
      value = next_value;
    endfunction
  endclass

  for (genvar i = 0; i < 96; ++i) begin : g
    bit a, b, shared_value;
    assign seen_a[i] = a;
    assign seen_b[i] = b;
    assign seen_shared[i] = shared_value;
    initial begin
      @(posedge clk_a);
      Writer::update(a);
      Writer::update(shared_value);
    end
    initial begin
      @(posedge clk_b);
      Writer::update(b);
      Writer::update(shared_value, 0);
    end
  end

  initial begin
    #1;
    if (all_a || all_b || seen_shared != '0) $stop;
    clk_a = 1;
    #1;
    if (!all_a || seen_b != '0 || seen_shared != '1) $stop;
    clk_b = 1;
    #1;
    if (!all_a || !all_b || seen_shared != '0) $stop;
    $display("*-* All Finished *-*");
    $finish;
  end
endmodule
