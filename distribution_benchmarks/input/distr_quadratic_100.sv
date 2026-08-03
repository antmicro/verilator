// 100 solutions via non-linear (quadratic) constraint
// x*x <= 10000 with x in [1:1000] => solutions are {1, 2, ..., 100}
// The constraint looks simple, but it is non-linear: the solver must determine
// that the upper bound is sqrt(10000) = 100.
// Expected: uniform over {1..100}, 1% each.

`ifndef NUM_ITERATIONS
  `define NUM_ITERATIONS 10000
`endif
module distr_quadratic_100;
  class C;
    rand int unsigned x;
    constraint c {
      x inside {[1:1000]};
      x * x <= 10000;
    }
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
