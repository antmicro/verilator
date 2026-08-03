// Distribution experiment: 1000 possible values
// Solutions: {0, 1, 2, ..., 999}
// Expected uniform distribution: 1% each
`ifndef NUM_ITERATIONS
  `define NUM_ITERATIONS 10000
`endif
module distr_range_1000;
  class C;
    rand bit [31:0] x;
    constraint c { x inside {[0:999]}; }
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
