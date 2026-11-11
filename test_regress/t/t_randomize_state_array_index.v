// DESCRIPTION: Verilator: Select a mutable state array element using a randomized index
//
// This file ONLY is placed under the Creative Commons Public Domain.
// SPDX-FileCopyrightText: 2026 The Verilator Authors
// SPDX-License-Identifier: CC0-1.0

// verilog_format: off
`define stop $stop
`define checkd(gotv,expv) do if ((gotv) !== (expv)) begin $write("%%Error: %s:%0d:  got=%0d exp=%0d\n", `__FILE__,`__LINE__, (gotv), (expv)); `stop; end while(0);
// verilog_format: on

class Lookup;
  int entries[4];
  int round;
  rand bit [1:0] key;
  rand int result;
  constraint choose {result == entries[key];}

  function void pre_randomize();
    ++round;
    foreach (entries[i]) entries[i] = 100 + (i ^ round);
  endfunction
endclass

module t;
  initial begin
    Lookup obj;
    int success;
    obj = new;
    for (int step = 1; step <= 2; ++step) begin
      // The same requested value has a different unique key after each update.
      success = obj.randomize() with {result == 102;};
      `checkd(success, 1);
      `checkd(int'(obj.key), 2 ^ step);
      `checkd(obj.result, 102);
      // The solver may choose a key, but every state array element must keep its value.
      foreach (obj.entries[i]) begin
        `checkd(obj.entries[i], 100 + (i ^ step));
      end
    end
    $write("*-* All Finished *-*\n");
    $finish;
  end
endmodule
