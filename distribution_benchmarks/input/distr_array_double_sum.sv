// Array-scaling benchmark: TWO independent sum constraints partitioning
// the array into separate coupled clusters -- tests whether a fix aimed
// at a single sum cluster (e.g. frequency-aware avoidance) generalizes
// when there are multiple, independently-coupled groups within one array.
//
// Variables:
//   arr[6] : bit [2:0] -- six 3-bit elements, domain [0:7] each
//
// Constraint:
//   arr[0]+arr[1]+arr[2] == 10  (cluster A)
//   arr[3]+arr[4]+arr[5] == 8   (cluster B)
//
// Solutions: 48 x 42 = 2016 (independent clusters multiply)
//
// Canonical output order: (arr[0],...,arr[5]) lexicographic ascending

`ifndef NUM_ITERATIONS
  `define NUM_ITERATIONS 10000
`endif

module distr_array_double_sum;
  class C;
    rand bit [2:0] arr[6];

    constraint c {
      {2'b0, arr[0]} + {2'b0, arr[1]} + {2'b0, arr[2]} == 5'd10;
      {2'b0, arr[3]} + {2'b0, arr[4]} + {2'b0, arr[5]} == 5'd8;
    }
  endclass

  initial begin
    automatic C c = new;
    repeat (`NUM_ITERATIONS) begin
      if (!bit'(c.randomize())) $display("-UNSAT");
      else $display("%0d %0d %0d %0d %0d %0d",
                    c.arr[0], c.arr[1], c.arr[2], c.arr[3], c.arr[4], c.arr[5]);
    end
    $finish;
  end
endmodule
