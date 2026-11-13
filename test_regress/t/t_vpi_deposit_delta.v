// DESCRIPTION: Verilator: Check VPI deposit, force, release, and fanout after the specified delays
//
// This file ONLY is placed under the Creative Commons Public Domain.
// SPDX-FileCopyrightText: 2026 <TODO>
// SPDX-License-Identifier: CC0-1.0

program p;
  import "DPI-C" context function int deposit(input string path, input int value);
  import "DPI-C" context function int force_value(input string path, input int value);
  import "DPI-C" context function int release_value(input string path);
  import "DPI-C" context function int read_value(input string path);

  initial begin
    #20;
    $root.t.hdl_driver = 7;
    if (0 == deposit("t.dut.driver", 'h6c)) $fatal;
    #0;
    if (read_value("t.dut.driver") != 'h6c) $stop;
    // Program #0 resumes in Re-Inactive. Sample module fanout after it settles.
    #1;
    if (read_value("t.dut.direct") != 'h6c || read_value("t.dut.chained") != 'h6c) $stop;
    if (read_value("t.dut.doubled") != 'hd8 || read_value("t.dut.delayed") != 'h0f) $stop;
    if (read_value("t.hdl_wire") != 7) $stop;
    #2;
    if (read_value("t.dut.delayed") != 'h6c) $stop;
    if (0 == deposit("t.dut.direct", 'h3c)) $fatal;
    #0;
    if (read_value("t.dut.direct") != 'h3c || read_value("t.dut.driver") != 'h6c) $stop;
    #1;
    if (read_value("t.dut.chained") != 'h3c) $stop;
    if (0 == force_value("t.dut.direct", 'h55)) $fatal;
    if (0 == deposit("t.dut.driver", 'ha5)) $fatal;
    #1;
    if (read_value("t.dut.direct") != 'h55 || read_value("t.dut.chained") != 'h55) $stop;
    if (0 == release_value("t.dut.direct")) $fatal;
    #1;
    if (read_value("t.dut.direct") != 'ha5 || read_value("t.dut.chained") != 'ha5) $stop;
    if (0 == force_value("t.dut.driver", 'h12)) $fatal;
    #1;
    if (read_value("t.dut.doubled") != 'h24 || read_value("t.dut.chained") != 'h12) $stop;
    if (0 == deposit("t.hdl_driver", 9)) $fatal;
    #1;
    if (read_value("t.hdl_wire") != 9) $stop;
    $root.t.program_done = 1;
    #100 $stop;
  end
endprogram

module connected;
  reg [7:0] driver /* verilator public_flat_rw */ /* verilator forceable */ = 'h0f;
  wire [7:0] direct /* verilator public_flat_rw */ /* verilator forceable */ = driver;
  wire [7:0] chained /* verilator public_flat_rw */ /* verilator forceable */ = direct;
  wire [7:0] doubled /* verilator public_flat_rw */ /* verilator forceable */ = driver * 2;
  wire [7:0] delayed /* verilator public_flat_rw */ /* verilator forceable */;
  assign #2 delayed = driver;
endmodule

module t;
  import "DPI-C" context function int deposit(input string path, input int value);

  byte unsigned q /* verilator public_flat_rw */ /* verilator forceable */ = 'h0f;
  wire [7:0] w /* verilator public_flat_rw */ /* verilator forceable */ = q;
  connected dut();
  reg [7:0] hdl_driver /* verilator public_flat_rw */ /* verilator forceable */ = 0;
  wire [7:0] hdl_wire /* verilator public_flat_rw */ /* verilator forceable */ = hdl_driver;
  bit program_done;
  p pr();

  always #100 q = 'hf0;

  initial begin
    #50;
    if (0 == deposit("t.q", 'h6c)) $fatal;
    #0;
    if (w !== 8'h6c) $fatal(1, "First driver deposit read back as %h", w);
    if (0 == deposit("t.w", 'h3c)) $fatal;
    #0;
    if (w !== 8'h3c) $fatal(1, "Wire deposit read back as %h", w);
    if (0 == deposit("t.q", 'ha5)) $fatal;
    #0;
    if (w !== 8'ha5) $fatal(1, "Second driver deposit read back as %h", w);
    if (!program_done) $stop;
    $write("*-* All Finished *-*\n");
    $finish;
  end
endmodule
