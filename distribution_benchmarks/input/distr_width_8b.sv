// Variables:
//   x : bit [7:0] -- 8-bit unsigned, domain [0:255]
//
// Constraint:
//   x inside {[0:9]}
//
// Solutions: {0, 1, 2, ..., 9} = 10

`ifndef NUM_ITERATIONS
  `define NUM_ITERATIONS 10000
`endif

module distr_width_8b;
  class C;
    rand bit [7:0] x;

    constraint x_c { x inside {[8'd0:8'd9]}; }
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
