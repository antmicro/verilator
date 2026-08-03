// DOCZYTAJ SPECKE W SUMIE NAJPIERW!!!!!!!!!!!!!!!!!!!!!!!!!!
// 10 possible (mode,value) pairs, conditional constraint
// mode=0 -> value in {0,1}      (2 options)
// mode=1 -> value in {2,3,4,5}  (4 options)
// mode=2 -> value in {6,7,8,9}  (4 options)
// Total: 10 solutions
// should be ~10% each if uniform
// howeverm, if solver picks mode first (uniform over {0,1,2}), then value:
//   mode=0 pairs get 1/3 * 1/2 = 16.7%
//   mode=1 pairs get 1/3 * 1/4 = 8.3%
//   mode=2 pairs get 1/3 * 1/4 = 8.3%  ---- meaning that its heavily biased?
`ifndef NUM_ITERATIONS
  `define NUM_ITERATIONS 10000
`endif
module distr_conditional_bias;
  class C;
    rand bit [1:0] mode;
    rand bit [3:0] value;
    constraint c {
      mode inside {[0:2]};
      if      (mode == 0) value inside {[0:1]};
      else if (mode == 1) value inside {[2:5]};
      else                value inside {[6:9]};
    }
  endclass

  initial begin
    automatic C c = new;
    repeat (`NUM_ITERATIONS) begin
      if (!bit'(c.randomize())) $display("-UNSAT");
      else $display("%0d %0d", c.mode, c.value);
    end
    $finish;
  end
endmodule
