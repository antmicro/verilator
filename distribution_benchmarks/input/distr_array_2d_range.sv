// Variables:
//   arr[2][4] : bit [1:0] -- eight independent 2-bit elements, domain [0:3] each
//
// Constraint:
//   arr[i][j] < 3 for all i,j  (each element independently in [0:2])
//
// Solutions: 3^8 = 6561

`ifndef NUM_ITERATIONS
  `define NUM_ITERATIONS 10000
`endif

module distr_array_2d_range;
  class C;
    rand bit [1:0] arr[2][4];

    constraint c {
      foreach (arr[i, j]) { arr[i][j] < 2'd3; }
    }
  endclass

  initial begin
    automatic C c = new;
    repeat (`NUM_ITERATIONS) begin
      if (!bit'(c.randomize())) $display("-UNSAT");
      else $display("%0d %0d %0d %0d %0d %0d %0d %0d",
                    c.arr[0][0], c.arr[0][1], c.arr[0][2], c.arr[0][3],
                    c.arr[1][0], c.arr[1][1], c.arr[1][2], c.arr[1][3]);
    end
    $finish;
  end
endmodule
