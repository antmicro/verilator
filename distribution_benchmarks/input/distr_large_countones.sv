// Solutions: C(23,6) = 100947


`ifndef NUM_ITERATIONS
  `define NUM_ITERATIONS 10000
`endif

module distr_countones_1e9;
  class C;
    rand bit [22:0] x;

    constraint cx { $countones(x) == 6; }
  endclass

  initial begin
    automatic C obj = new;
    repeat (`NUM_ITERATIONS) begin
      if (!bit'(obj.randomize())) $display("-UNSAT");
      else $display("%0d", obj.x);
    end
    $finish;
  end
endmodule
