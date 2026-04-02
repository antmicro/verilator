// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain.
// SPDX-FileCopyrightText: 2026 Antmicro
// SPDX-License-Identifier: CC0-1.0

module t;

  bit clk = 0;
  initial repeat(100) #1 clk ^= 1;

  bit a = 0;

  assert property (@(posedge clk) 1)
    begin
      // meant to be scheduled within observed section
      if(a == 1 && $sampled(a) == 0) begin
        $write("*-* All Finished *-*\n");
        $finish;
      end
      else begin
        $display("actual:   a=%d, $sampled(a)=%d", a, $sampled(a));
        $display("expected: a=1, $sampled(a)=0");
        $stop;
      end
    end

  always @(posedge clk) begin
    a = 1; // should execute before concurent assert body
  end
endmodule

