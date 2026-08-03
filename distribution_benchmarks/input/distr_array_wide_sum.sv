// Variables:
//   arr[4] : bit [3:0] -- four 4-bit elements, domain [0:15] each
//
// Constraint:
//   arr[0] + arr[1] + arr[2] + arr[3] == 30  (widened to avoid wraparound)
//
// Solutions: 2736 (all (a,b,c,d) in [0:15]^4 with a+b+c+d==30)

`ifndef NUM_ITERATIONS
  `define NUM_ITERATIONS 10000
`endif

module distr_array_wide_sum;
  class C;
    rand bit [3:0] arr[4];

    constraint c {
      {2'b0, arr[0]} + {2'b0, arr[1]} + {2'b0, arr[2]} + {2'b0, arr[3]} == 6'd30;
    }
  endclass

  initial begin
    automatic C c = new;
    repeat (`NUM_ITERATIONS) begin
      if (!bit'(c.randomize())) $display("-UNSAT");
      else $display("%0d %0d %0d %0d", c.arr[0], c.arr[1], c.arr[2], c.arr[3]);
    end
    $finish;
  end
endmodule
