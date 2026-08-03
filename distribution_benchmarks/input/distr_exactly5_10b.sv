// Variables:
//   b0..b9 : bit -- ten independent 1-bit rand variables
//
// Constraint:
//   b0 + b1 + b2 + b3 + b4 + b5 + b6 + b7 + b8 + b9 == 5
//
// Solutions: C(10,5) = 252

`ifndef NUM_ITERATIONS
  `define NUM_ITERATIONS 10000
`endif

module distr_exactly5_10b;
  class C;
    rand bit b0, b1, b2, b3, b4, b5, b6, b7, b8, b9;

    constraint sum_c {
      {3'b0, b0} + {3'b0, b1} + {3'b0, b2} + {3'b0, b3} + {3'b0, b4} +
      {3'b0, b5} + {3'b0, b6} + {3'b0, b7} + {3'b0, b8} + {3'b0, b9} == 4'd5;
    }
  endclass

  initial begin
    automatic C c = new;
    repeat (`NUM_ITERATIONS) begin
      if (!bit'(c.randomize())) $display("-UNSAT");
      else $display("%0d %0d %0d %0d %0d %0d %0d %0d %0d %0d",
               c.b0, c.b1, c.b2, c.b3, c.b4, c.b5, c.b6, c.b7, c.b8, c.b9);
    end
    $finish;
  end
endmodule
