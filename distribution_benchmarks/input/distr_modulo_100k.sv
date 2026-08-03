// Large modular constraint: every seventh value in a wide domain.
// Dimension 2 (solution space size): ~100k solutions.
// Dimension 5 (variable width): 20-bit variable.
// Paired with distr_modulo_1000 (same constraint type, 16b, ~1k solutions)
// to isolate the effect of solution space size on sampling quality.
//
// Variables:
//   x : bit [19:0] -- 20-bit unsigned, domain [0:1048575]
//
// Constraints:
//   x inside {[1:700000]}
//   x % 7 == 0
//
// Solutions: {7, 14, 21, ..., 700000} = 100000
// No overflow: 700000 < 2^20 = 1048576
//
// Canonical output order: ascending integer value of x

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
