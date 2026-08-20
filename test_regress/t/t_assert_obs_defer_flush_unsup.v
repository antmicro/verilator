// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain.
// SPDX-FileCopyrightText: 2026 Antmicro
// SPDX-License-Identifier: CC0-1.0

module t;
  event e;
  logic a;
  logic q;

  initial begin
    assert #0 (0)
    else $display("event control");
    @e;
  end

  initial begin
    assert #0 (0)
    else $display("wait");
    wait (a);
  end

  initial begin
    fork
      begin
      end
    join_none
    assert #0 (0)
    else $display("wait fork");
    wait fork;
  end

  initial begin : disable_block
    assert #0 (0)
    else $display("disable");
    disable disable_block;
  end

  initial begin
    assert #0 (0)
    else $display("zero delay");
    #0;
  end

  always_comb begin
    assert #0 (a)
    else $display("always_comb");
  end

  always_latch begin
    if (a) begin
      q = 1'b1;
      assert #0 (q)
      else $display("always_latch");
    end
  end
endmodule
