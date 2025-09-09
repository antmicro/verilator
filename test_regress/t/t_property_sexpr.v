// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed into the Public Domain, for any use,
// without warranty, 2025 by Antmicro.
// SPDX-License-Identifier: CC0-1.0

module t;
  logic clk = 0;
  initial forever #1 clk = ~clk;
  integer cyc = 1;
  bit [3:0] val = 0;
  event e1;
  event e2;
  event e3;
  event e4;
  event e5;
  event e6;
  event e7;
  event e8;

  always @(negedge clk) begin
    val <= 4'(cyc % 4);

    if (cyc >= 0 && cyc <= 4) begin
      ->e1;
`ifdef TEST_VERBOSE
      $display("[%0t] triggered e1", $time);
`endif
    end
    if (cyc >= 5 && cyc <= 10) begin
      ->e2;
`ifdef TEST_VERBOSE
      $display("[%0t] triggered e2", $time);
`endif
    end
    if (cyc >= 11 && cyc <= 15) begin
      ->e3;
`ifdef TEST_VERBOSE
      $display("[%0t] triggered e3", $time);
`endif
    end
    if (cyc >= 16 && cyc <= 20) begin
      ->e4;
`ifdef TEST_VERBOSE
      $display("[%0t] triggered e4", $time);
`endif
    end
    if (cyc >= 21 && cyc <= 25) begin
      ->e5;
`ifdef TEST_VERBOSE
      $display("[%0t] triggered e5", $time);
`endif
    end
    if (cyc >= 26 && cyc <= 30) begin
      ->e6;
`ifdef TEST_VERBOSE
      $display("[%0t] triggered e6", $time);
`endif
    end
    if (cyc >= 31 && cyc <= 35) begin
      ->e7;
`ifdef TEST_VERBOSE
      $display("[%0t] triggered e7", $time);
`endif
    end
    if (cyc >= 36 && cyc <= 40) begin
      ->e8;
`ifdef TEST_VERBOSE
      $display("[%0t] triggered e8", $time);
`endif
    end
`ifdef TEST_VERBOSE
    $display("cyc=%0d val=%0d", cyc, val);
`endif
    cyc <= cyc + 1;
    if (cyc == 100) begin
      $write("*-* All Finished *-*\n");
      $finish;
    end
  end

  assert property (@(e1) ##1 1)
    $display("[%0t] single delay with const stmt, fileline:%d", $time, `__LINE__);

  assert property (@(e2) ##1 val[0])
    $display("[%0t] single delay with var stmt, fileline:%d", $time, `__LINE__);
  else $display("[%0t] single delay with var else, fileline:%d", $time, `__LINE__);

  assert property (@(e3) ##2 val[0])
    $display("[%0t] single multi-cycle delay with var stmt, fileline:%d", $time, `__LINE__);
  else $display("[%0t] single multi-cycle delay with var else, fileline:%d", $time, `__LINE__);

  assert property (@(e4) ##1 (val[0]))
    $display("[%0t] single delay with var brackets 1 stmt, fileline:%d", $time, `__LINE__);
  else $display("[%0t] single delay with var brackets 1 else, fileline:%d", $time, `__LINE__);

  assert property (@(e5) (##1 (val[0])))
    $display("[%0t] single delay with var brackets 2 stmt, fileline:%d", $time, `__LINE__);
  else $display("[%0t] single delay with var brackets 2 else, fileline:%d", $time, `__LINE__);

  assert property (@(e6) (##1 val[0] && val[1]))
    $display("[%0t] single delay with negated var stmt, fileline:%d", $time, `__LINE__);
  else $display("[%0t] single delay with negated var else, fileline:%d", $time, `__LINE__);

  assert property (@(e7) not ##1 val[0])
    $display("[%0t] single delay with negated var stmt, fileline:%d", $time, `__LINE__);
  else $display("[%0t] single delay with negated var else, fileline:%d", $time, `__LINE__);

  assert property (@(e8) not (##1 val[0]))
    $display("[%0t] single delay with negated var brackets stmt, fileline:%d", $time, `__LINE__);
  else $display("[%0t] single delay with negated var brackets else, fileline:%d", $time, `__LINE__);
endmodule
