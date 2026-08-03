// ~120 solutions
// x in [1000:9999], digit sum of x == 8
// Solutions are 4-digit numbers like: 1007, 1016, 1025, 1034, ..., 8000
// Expected: uniform over all ~120 valid numbers.
// Q: Does the Verilator finds all solutions, or cluster around a subset?

`ifndef NUM_ITERATIONS
  `define NUM_ITERATIONS 10000
`endif

module distr_digit_sum;
  class C;
    rand int unsigned x;
    constraint c {
      x inside {[1000:9999]};
      (x / 1000) + (x / 100) % 10 + (x / 10) % 10 + (x % 10) == 8;
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
