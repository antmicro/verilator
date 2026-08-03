// Variables:
//   mode : bit     -- 1-bit selector (0 = small bin, 1 = large bin)
//   x    : bit [15:0] -- 16-bit unsigned value
//
// Constraint:
//   if (mode == 0) x inside {[0:99]}    -- small bin:  100 solutions
//   else           x inside {[0:9999]}  -- large bin: 10000 solutions
//
// Solutions: 100 + 10000 = 10100
// Bin ratio: 100x (large bin is 100x bigger than small bin)


`ifndef NUM_ITERATIONS
  `define NUM_ITERATIONS 10000
`endif

module distr_unequal_bins;
  class C;
    rand bit       mode;
    rand bit [15:0] x;

    constraint x_c {
      if (mode == 1'b0)
        x inside {[16'd0:16'd99]};
      else
        x inside {[16'd0:16'd9999]};
    }
  endclass

  initial begin
    automatic C c = new;
    repeat (`NUM_ITERATIONS) begin
      if (!bit'(c.randomize())) $display("-UNSAT");
      else $display("%0d %0d", c.mode, c.x);
    end
    $finish;
  end
endmodule
