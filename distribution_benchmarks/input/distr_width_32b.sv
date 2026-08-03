// Variables:
//   x : int unsigned -- 32-bit unsigned, domain [0:2^32-1]
//
// Constraint:
//   x inside {[0:9]}
//
// Solutions: {0, 1, 2, ..., 9} = 10

`ifndef NUM_ITERATIONS
  `define NUM_ITERATIONS 10000
`endif

module distr_width_32b;
  class C;
    rand int unsigned x;

    constraint x_c { x inside {[32'd0:32'd9]}; }
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
