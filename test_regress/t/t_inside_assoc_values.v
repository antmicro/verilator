// DESCRIPTION: Verilator: Match the values of present associative array elements
//
// This file ONLY is placed under the Creative Commons Public Domain.
// SPDX-FileCopyrightText: 2020 Wilson Snyder
// SPDX-License-Identifier: CC0-1.0

// verilog_format: off
`define stop $stop
`define checkd(gotv,expv) do if ((gotv) !== (expv)) begin $write("%%Error: %s:%0d:  got=%0d exp=%0d\n", `__FILE__,`__LINE__, (gotv), (expv)); `stop; end while(0);
// verilog_format: on

module t;
  typedef bit [96:0] index_t;

  initial begin
    bit [6:0] values[index_t];
    bit [6:0] tags[string];
    bit [6:0] by_index[*];
    index_t low, high;
    values = '{default: 7'd52};
    low = 97'd9;
    high = {1'b1, 96'd9};

    // A missing index reads the default but contributes no value to the set.
    `checkd(values[low], 7'd52);
    `checkd((7'd52 inside {values}), 1'b0);
    values[low] = 7'd17;
    values[high] = 7'd41;
    `checkd((7'd9 inside {values}), 1'b0);
    `checkd((7'd17 inside {values}), 1'b1);
    `checkd((7'd41 inside {values}), 1'b1);
    `checkd((9'h111 inside {values}), 1'b0);

    // Deleting and replacing entries must change membership immediately.
    values.delete(low);
    `checkd((7'd17 inside {values}), 1'b0);
    values[high] = 7'd52;
    `checkd((7'd41 inside {values}), 1'b0);
    `checkd((7'd52 inside {values}), 1'b1);
    tags["first"] = 7'd19;
    `checkd((7'd19 inside {tags}), 1'b1);
    tags.delete();
    `checkd((7'd19 inside {tags}), 1'b0);
    // A wildcard index may be wider than an int; only its stored value participates.
    by_index[high] = 7'd67;
    `checkd((7'd67 inside {by_index}), 1'b1);
    `checkd((7'd9 inside {by_index}), 1'b0);
    $write("*-* All Finished *-*\n");
    $finish;
  end
endmodule
