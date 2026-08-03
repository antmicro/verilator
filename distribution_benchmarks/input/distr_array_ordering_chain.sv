// Array-scaling benchmark: strictly-increasing chain constraint across
// array elements -- a genuinely different coupling shape than sum
// (order-dependent, not commutative), and one that showed up as
// intrinsically hard for solvers in Verilator's own regression tests
// (t_constraint_unpacked_array.v's 3D ordering-chain case).
//
// Variables:
//   arr[4] : bit [2:0] -- four 3-bit elements, domain [0:7] each
//
// Constraint:
//   arr[0] < arr[1] < arr[2] < arr[3]  (strictly increasing)
//
// Solutions: C(8,4) = 70 (any 4-subset of [0:7], in increasing order)
//
// Canonical output order: (arr[0],arr[1],arr[2],arr[3]) lexicographic ascending

`ifndef NUM_ITERATIONS
  `define NUM_ITERATIONS 10000
`endif

module distr_array_ordering_chain;
  class C;
    rand bit [2:0] arr[4];

    constraint c {
      arr[0] < arr[1];
      arr[1] < arr[2];
      arr[2] < arr[3];
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
