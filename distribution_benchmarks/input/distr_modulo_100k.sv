// Variables:
//   x : bit [19:0] -- 20-bit unsigned, domain [0:1048575]
//
// Constraints:
//   x inside {[1:700000]}
//   x % 7 == 0
//
// Solutions: {7, 14, 21, ..., 700000} = 100000


`ifndef NUM_ITERATIONS
  `define NUM_ITERATIONS 10000
`endif

module distr_modulo_100k;
  class C;
    rand bit [19:0] x;

    constraint range_c  { x inside {[20'd1:20'd700000]}; }
    constraint modulo_c { x % 20'd7 == 20'd0; }
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
