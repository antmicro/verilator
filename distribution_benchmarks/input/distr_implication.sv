// DOCZYTAJ SPECKE!!!!
// 3 possible values, implication constraint
// Constraint: a -> b  (if a==1 then b must be 1)
// Solutions: (a=0,b=0)  (a=0,b=1)  (a=1,b=1)
// Expected uniform: 33.3% each
// Naive rejection sampling would give: 25% 25% 50% -- bias!
//
`ifndef NUM_ITERATIONS
  `define NUM_ITERATIONS 10000
`endif
module distr_implication;
  class C;
    rand bit a;
    rand bit b;
    constraint c { a -> b; }
  endclass

  initial begin
    automatic C c = new;
    repeat (`NUM_ITERATIONS) begin
      if (!bit'(c.randomize())) $display("-UNSAT");
      else $display("%0d %0d", c.a, c.b);
    end
    $finish;
  end
endmodule
