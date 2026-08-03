// Variables:
//   arr[3] : bit [2:0] -- three 3-bit elements, domain [0:7] each
//
// Constraint:
//   arr[0] + arr[1] + arr[2] == 10  (widened to avoid wraparound)
//
// Solutions: 48 (all (a,b,c) in [0:7]^3 with a+b+c==10)

`ifndef NUM_ITERATIONS
  `define NUM_ITERATIONS 10000
`endif

module distr_array_sum;
  class C;
    rand bit [2:0] arr[3];

    constraint c {
      {2'b0, arr[0]} + {2'b0, arr[1]} + {2'b0, arr[2]} == 5'd10;
    }
  endclass

  initial begin
    automatic C c = new;
    repeat (`NUM_ITERATIONS) begin
      if (!bit'(c.randomize())) $display("-UNSAT");
      else $display("%0d %0d %0d", c.arr[0], c.arr[1], c.arr[2]);
    end
    $finish;
  end
endmodule
