// Distribution experiment: 100 possible values
// Solutions: {0, 1, 2, ..., 99}
// Expected uniform distribution: 1% each
// does Verilator distribute uniformly over a large flat range?
`ifndef NUM_ITERATIONS
  `define NUM_ITERATIONS 10000
`endif
module distr_range_100;
  class C;
    rand bit [6:0] x;
    constraint c { x inside {[0:99]}; }
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
