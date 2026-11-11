// DESCRIPTION: Verilator: Resize rand queues and dynamic arrays using the active size constraint
//
// This file ONLY is placed under the Creative Commons Public Domain.
// SPDX-FileCopyrightText: 2026 The Verilator Authors
// SPDX-License-Identifier: CC0-1.0

// verilog_format: off
`define stop $stop
`define checkd(gotv,expv) do if ((gotv) !== (expv)) begin $write("%%Error: %s:%0d:  got=%0d exp=%0d\n", `__FILE__,`__LINE__, (gotv), (expv)); `stop; end while(0);
// verilog_format: on

module t;
  class Row;
    rand bit [6:0] values[$];
  endclass

  class SizedRow;
    rand bit [6:0] values[];
    constraint single {values.size() == 1;}
    constraint triple {values.size() == 3;}
    constraint contents {foreach (values[i]) values[i] == 7'(21 + i);}
  endclass

  initial begin
    Row row;
    SizedRow sized;
    int success;
    row = new;

    // The two calls resize the same physical queue. Check the solved entries after each resize.
    repeat (2) begin
      success = row.randomize() with {
        values.size() == 3;
        foreach (values[i]) values[i] == 7'(11 + i);
      };
      `checkd(success, 1);
      `checkd(row.values.size(), 3);
      foreach (row.values[i]) `checkd(row.values[i], 7'(11 + i));
      success = row.randomize() with {
        values.size() == 1;
        values[0] == 7'd53;
      };
      `checkd(success, 1);
      `checkd(row.values.size(), 1);
      `checkd(row.values[0], 7'd53);
    end
    // Each independently enabled declaration must resize the dynamic array.
    sized = new;
    sized.single.constraint_mode(0);
    success = sized.randomize();
    `checkd(success, 1);
    `checkd(sized.values.size(), 3);
    foreach (sized.values[i]) `checkd(sized.values[i], 7'(21 + i));
    sized.single.constraint_mode(1);
    sized.triple.constraint_mode(0);
    success = sized.randomize();
    `checkd(success, 1);
    `checkd(sized.values.size(), 1);
    `checkd(sized.values[0], 7'd21);
    $write("*-* All Finished *-*\n");
    $finish;
  end
endmodule
