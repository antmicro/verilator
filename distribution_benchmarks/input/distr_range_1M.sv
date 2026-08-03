// Trivial large range: 1 million consecutive values.
// Dimension 2 (solution space size): 1M solutions.
// Dimension 5 (variable width): 20-bit variable.
// Paired with distr_range_10 / 100 / 1000 to complete the solution space
// size series with a trivial (plain range) constraint at large scale.
//
// Variables:
//   x : bit [19:0] -- 20-bit unsigned, domain [0:1048575]
//
// Constraint:
//   x inside {[0:999999]}
//
// Solutions: {0, 1, 2, ..., 999999} = 1000000
// No overflow: 999999 < 2^20 = 1048576
//
// Canonical output order: ascending integer value of x

`ifndef NUM_ITERATIONS
  `define NUM_ITERATIONS 10000
`endif

module distr_range_1M;
  class C;
    rand bit [19:0] x;

    constraint x_c { x inside {[20'd0:20'd999999]}; }
  endclass

  initial begin
    automatic C c = new;
    repeat (`NUM_ITERATIONS) begin
      if (!bit'(c.randomize())) $display("-UNSAT");
      else $display("%0d", c.x);
    end
    $finish;
  end
endmodule
