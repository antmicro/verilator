// Array-scaling benchmark: many independent array elements (double the
// element count of distr_array_basic), stressing the "lots of vars" axis --
// each flattened array element becomes its own SMT variable/neighbor-flip
// candidate, so this exercises how both samplers scale with element count.
//
// Variables:
//   arr[6] : bit [2:0] -- six independent 3-bit elements, domain [0:7] each
//
// Constraint:
//   arr[i] < 5 for all i  (each element independently in [0:4])
//
// Solutions: 5^6 = 15625
//
// Canonical output order: (arr[0],...,arr[5]) lexicographic ascending

`ifndef NUM_ITERATIONS
  `define NUM_ITERATIONS 10000
`endif

module distr_array_many_elem;
  class C;
    rand bit [2:0] arr[6];

    constraint c {
      foreach (arr[i]) { arr[i] < 3'd5; }
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
