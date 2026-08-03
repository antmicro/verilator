// Width-isolation benchmark: 4-bit variable, 10 solutions.
// Paired with distr_width_8b / 16b / 32b: same logical constraint
// x inside {[0:9]}, only the bitvector width in SMT-LIB changes.
// Isolates the effect of variable width on sampling distribution.
//
// Variables:
//   x : bit [3:0] -- 4-bit unsigned, domain [0:15]
//
// Constraint:
//   x inside {[0:9]}
//
// Solutions: {0, 1, 2, ..., 9} = 10
//
// Canonical output order: ascending integer value of x

`ifndef NUM_ITERATIONS
  `define NUM_ITERATIONS 10000
`endif

module distr_width_4b;
  class C;
    rand bit [3:0] x;

    constraint x_c { x inside {[4'd0:4'd9]}; }
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
