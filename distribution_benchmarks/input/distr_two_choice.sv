// Distribution experiment: 2 possible values
// Solutions: {42, 137}
// Expected uniform distribution: 50% each
`ifndef NUM_ITERATIONS
  `define NUM_ITERATIONS 10000
`endif
module distr_two_choice;
  class C;
    rand bit [7:0] x;
    constraint c { x inside {42, 137}; }
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
