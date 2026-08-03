// 10 possible values
// Solutions: {1, 2, 3, 4, 5, 6, 7, 8, 9, 10}
// Expected uniform distribution: 10% each
`ifndef NUM_ITERATIONS
  `define NUM_ITERATIONS 10000
`endif
module distr_range_10;
  class C;
    rand bit [4:0] x;
    constraint c { x inside {[1:10]}; }
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
