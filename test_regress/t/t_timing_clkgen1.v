// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain.
// SPDX-FileCopyrightText: 2020 Wilson Snyder
// SPDX-License-Identifier: CC0-1.0

module clkgen(output bit clk);
  initial begin
    #(8.0:5:3) clk = 1;  // Middle is default
    forever begin
      #5 clk = ~clk;
    end
  end
endmodule

module t;
  wire logic clk;
  time first_posedge = 5;

  clkgen clkgen (.clk);

  initial begin
    if ($test$plusargs("mindelays")) begin
      first_posedge = 8;
    end else if ($test$plusargs("maxdelays")) begin
      first_posedge = 3;
    end
  end

  int  cyc;
  always @ (posedge clk) begin
    cyc <= cyc + 1;
`ifdef TEST_VERBOSE
    $display("[%0t] cyc=%0d", $time, cyc);
`endif
    if (cyc == 0) begin
      if ($time != first_posedge) $stop;
    end
    else if (cyc == 1) begin
      if ($time != first_posedge + 10) $stop;
    end
    else if (cyc == 2) begin
      if ($time != first_posedge + 20) $stop;
    end
    else if (cyc == 9) begin
      $write("*-* All Finished *-*\n");
      $finish;
    end
  end
endmodule
