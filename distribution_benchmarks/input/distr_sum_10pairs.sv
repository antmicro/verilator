// Distribution experiment: 10 possible (x,y) pairs
// Constraint: x + y == 11, both in [1:10]
// Solutions: (1,10)(2,9)(3,8)(4,7)(5,6)(6,5)(7,4)(8,3)(9,2)(10,1)
// Expected uniform: 10% each
`ifndef NUM_ITERATIONS
  `define NUM_ITERATIONS 10000
`endif
module distr_sum_10pairs;
  class C;
    rand bit [4:0] x;
    rand bit [4:0] y;
    constraint c {
      x inside {[1:10]};
      y inside {[1:10]};
      x + y == 11;
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
