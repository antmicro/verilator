// Array-isolation benchmark: array elements mixed with a plain scalar var
// in the same class, linked by a conditional (if/else) constraint --
// different constraint *shape* than the independent-range (distr_array_basic)
// or sum (distr_array_sum) cases, and exercises m_vars containing both array
// and non-array variables together in one solve.
//
// NOTE: an earlier version of this benchmark used a whole-array `unique {arr}`
// constraint instead of `sel`, but was dropped: Verilator's own regression
// test (t_constraint_unq_arr.v) documents that "Z3 does not actually give
// unique elements (bug?) as of Jul 2026" for small-domain unique-array
// constraints, and only avoids exposing it by using a domain far too large
// to fully enumerate here (16-bit, 4 elements). That's a pre-existing
// SMT-encoding gap independent of the sampler algorithm, not something
// fixable from the sampler side, so it doesn't belong in this comparison.
//
// Variables:
//   arr[2] : bit [2:0] -- two independent 3-bit elements, domain [0:7] each
//   sel    : bit       -- selects which ordering constraint applies
//
// Constraint:
//   sel==1 -> arr[0] >  arr[1]
//   sel==0 -> arr[0] <= arr[1]
//
// Solutions: 64 (every (arr[0],arr[1]) pair in [0:7]^2, paired with whichever
// sel value its ordering satisfies -- sel is fully determined by (arr[0],arr[1]))
//
// Canonical output order: (sel,arr[0],arr[1]) lexicographic ascending
// Output format: "%0d %0d %0d" sel arr[0] arr[1]

`ifndef NUM_ITERATIONS
  `define NUM_ITERATIONS 10000
`endif

module distr_array_mixed;
  class C;
    rand bit [2:0] arr[2];
    rand bit sel;

    constraint c {
      if (sel) { arr[0] > arr[1]; }
      else { arr[0] <= arr[1]; }
    }
  endclass

  initial begin
    automatic C c = new;
    repeat (`NUM_ITERATIONS) begin
      if (!bit'(c.randomize())) $display("-UNSAT");
      else $display("%0d %0d %0d", c.sel, c.arr[0], c.arr[1]);
    end
    $finish;
  end
endmodule
