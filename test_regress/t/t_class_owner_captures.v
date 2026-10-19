// DESCRIPTION: Verilator: Minimal capture layout, copying and selective inlining
//
// This file ONLY is placed under the Creative Commons Public Domain.
// SPDX-FileCopyrightText: 2026 Wilson Snyder
// SPDX-License-Identifier: CC0-1.0

// Check direct capture layout, shallow copying and selective module inlining.
// Indirect writes must also schedule the captured targets' combinational logic.

// verilog_format: off
`define stop $stop
`define checkd(gotv,expv) do if ((gotv) !== (expv)) begin $write("%%Error: %s:%0d: %m got=%0d exp=%0d\n", `__FILE__,`__LINE__, (gotv), (expv)); `stop; end while(0)
// verilog_format: on

module captured_target;
  int value;
  int doubled;
  // A class write to value must trigger this logic before the later sum check.
  always_comb doubled = 2 * value;
endmodule

module uncaptured_neighbor (
    input int value,
    output int result
);
  // The driver checks that preserving captured targets does not prevent this inlining.
  assign result = value + 3;
endmodule

module capture_pair_owner;
  // One definition, two targets: the driver checks exactly two direct capture fields.
  captured_target left (), right ();
  class Reader;
    function void set_values(int l, int r);
      left.value = l;
      right.value = r;
    endfunction
    function int sum();
      return left.doubled + right.doubled;
    endfunction
  endclass
  Reader c = new;
  Reader copied;
endmodule

module capture_pair_case (
    output bit done
);
  // Share the Reader implementation while binding its pointers to different children.
  capture_pair_owner first (), second ();
  int value, result;
  uncaptured_neighbor neighbor (
      value,
      result
  );
  initial begin
    first.c.set_values(1, 10);
    second.c.set_values(2, 20);
    value = 4;
    #1;
    `checkd(first.c.sum(), 22);
    `checkd(second.c.sum(), 44);
    `checkd(result, 7);
    // A shallow copy must retain the same model-lifetime targets, not copy their state.
    first.copied = new first.c;
    first.c.set_values(3, 30);
    value = 8;
    #1;
    // The copy sees first's updated targets; second's independently bound targets do not.
    `checkd(first.copied.sum(), 66);
    `checkd(second.c.sum(), 44);
    `checkd(result, 11);
    done = 1;
  end
endmodule

module t;
  wire [0:0] done;
  capture_pair_case capture_pairs (done[0]);

  initial begin
    wait (&done);
    $write("*-* All Finished *-*\n");
    $finish;
  end
  initial begin
    #100;
    $error("Incomplete scenarios: done=%b", done);
    `stop;
  end
endmodule
