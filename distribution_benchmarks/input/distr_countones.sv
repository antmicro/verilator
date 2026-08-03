// 70 non-obvious solutions via bit-count constraint
// x in [0:255] (8 bits), exactly 4 bits must be set => C(8,4) = 70 solutions
// Solutions are scattered non-contiguously across [0:255]: 15, 23, 27, 29, 30, 39, 43, ...
// Solution space density: 70/256 ~= 27% -- moderately dense
// Q: Does Verilator bias toward some patterns, e.g. with set bits clustered
// at low positions (0b00001111=15), or e.g. spread out (0b10001011=139)?
// each solution should be 1/70

`ifndef NUM_ITERATIONS
  `define NUM_ITERATIONS 10000
`endif

module distr_countones;
  class C;
    rand bit [7:0] x;
    constraint c { $countones(x) == 4; }
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
