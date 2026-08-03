// Variables:
//   x : bit [15:0] -- 16-bit unsigned, domain [0:65535]
//
// Constraints:
//   x inside {[1:5000]}
//   x % 5 == 0
//
// Solutions: {5, 10, 15, ..., 5000} = 1000

`ifndef NUM_ITERATIONS
  `define NUM_ITERATIONS 10000
`endif

module distr_modulo_1000;
  class C;
    rand bit [15:0] x;

    constraint range_c  { x inside {[16'd1:16'd5000]}; }
    constraint modulo_c { x % 16'd5 == 16'd0; }
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
