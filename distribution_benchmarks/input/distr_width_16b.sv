// Width-isolation benchmark: 16-bit variable, 10 solutions.
// Paired with distr_width_4b / 8b / 32b: same logical constraint
// x inside {[0:9]}, only the bitvector width in SMT-LIB changes.
// Isolates the effect of variable width on sampling distribution.
//
// Variables:
//   x : bit [15:0] -- 16-bit unsigned, domain [0:65535]
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

module distr_width_16b;
  class C;
    rand bit [15:0] x;

    constraint x_c { x inside {[16'd0:16'd9]}; }
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
