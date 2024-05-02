// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain, for
// any use, without warranty, 2024.
// SPDX-License-Identifier: CC0-1.0

module t;
  wire inoutSignal;

  submod foo (inoutSignal);

  initial begin
    #2 $display("*-* All Finished *-*");
    $finish();
  end
endmodule

module submod (
    inoutSignal
);
  inout inoutSignal;

  reg cond1;
  reg cond2;

  initial begin
    cond1 = 1;
    cond2 = 0;
  end

  task testTask;
    begin
      #1;
    end
  endtask

  // This always is put under an 'initial' active
  always @(inoutSignal) begin
    if (cond1) begin
      if (cond2) begin
        testTask;
      end
    end
  end
endmodule
