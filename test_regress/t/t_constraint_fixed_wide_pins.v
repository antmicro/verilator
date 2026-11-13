// DESCRIPTION: Verilator: Preserve coupled scalar pin targets beside a fixed wide value
//
// This file ONLY is placed under the Creative Commons Public Domain
// SPDX-FileCopyrightText: 2026 The Verilator Authors
// SPDX-License-Identifier: CC0-1.0

class Sample;
  rand bit [128:0] fixed_value;
  rand bit [95:0] choice;
  rand bit [95:0] complement;
  rand bit [95:0] changing;
  bit [128:0] required;
  bit [95:0] mask;
  constraint legal {
    fixed_value == required;
    choice inside {96'h100000000000000000000003, 96'h800000000000000000000005};
    complement == ~choice;
    changing == (choice ^ mask);
  }
endclass

module t;
  initial begin
    automatic Sample s = new;
    bit [95:0] previous;
    s.required = {65'h123456789acdef01, 64'hfecba9876543210};
    for (int i = 0; i < 10; i++) begin
      s.mask = ~s.mask;
      if (i == 0) begin
        if (0 == s.randomize()) $stop;
      end else begin
        if (0 == s.randomize() with {choice != local::previous;}) $stop;
      end
      if (s.fixed_value !== s.required || s.complement !== ~s.choice
          || s.changing !== (s.choice ^ s.mask)) $stop;
      if (!(s.choice inside {96'h100000000000000000000003, 96'h800000000000000000000005})) $stop;
      if (i > 0 && s.choice == previous) $stop;
      previous = s.choice;
      s.required = ~s.required;
    end
    $display("*-* All Finished *-*");
    $finish;
  end
endmodule
