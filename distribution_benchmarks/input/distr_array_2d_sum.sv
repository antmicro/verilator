// Variables:
//   arr[2][2] : bit [2:0] -- four 3-bit elements, domain [0:7] each
//
// Constraint:
//   arr[0][0]+arr[0][1]+arr[1][0]+arr[1][1] == 10
//
// Solutions: 246

`ifndef NUM_ITERATIONS
  `define NUM_ITERATIONS 10000
`endif

module distr_array_2d_sum;
  class C;
    rand bit [2:0] arr[2][2];

    constraint c {
      {2'b0, arr[0][0]} + {2'b0, arr[0][1]} + {2'b0, arr[1][0]} + {2'b0, arr[1][1]} == 6'd10;
    }
  endclass

  initial begin
    automatic C c = new;
    repeat (`NUM_ITERATIONS) begin
      if (!bit'(c.randomize())) $display("-UNSAT");
      else $display("%0d %0d %0d %0d",
                    c.arr[0][0], c.arr[0][1], c.arr[1][0], c.arr[1][1]);
    end
    $finish;
  end
endmodule
