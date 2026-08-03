// 100 solutions split across two symmetric groups
// |x - y| == 50, x,y in [1:100]
// group A (x > y): (51,1)(52,2)...(100,50)  -- 50 pairs
// grp B (y > x): (1,51)(2,52)...(50,100)  -- 50 pairs
// Total: 100 solutions

`ifndef NUM_ITERATIONS
  `define NUM_ITERATIONS 10000
`endif

module distr_abs_diff_100;
  class C;
    rand bit [6:0] x;
    rand bit [6:0] y;
    constraint c {
      x inside {[1:100]};
      y inside {[1:100]};
      (x > y ? x - y : y - x) == 50;
    }
  endclass

  initial begin
    automatic C c = new;
    repeat (`NUM_ITERATIONS) begin
      if (!bit'(c.randomize())) $display("-UNSAT");
      else $display("%0d %0d", c.x, c.y);
    end
    $finish;
  end
endmodule
