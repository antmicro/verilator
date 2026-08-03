// Array-scaling benchmark: associative array (new array TYPE for this
// suite -- only fixed-size unpacked arrays tested so far), holding a sum
// constraint over its populated entries.
//
// Variables:
//   aa[int] : int -- associative array, 3 populated keys (10, 20, 30),
//             each value's domain restricted to [0:7]
//
// Constraint:
//   aa[10] + aa[20] + aa[30] == 15, each in [0:7]
//
// Solutions: 28 (all (a,b,c) in [0:7]^3 with a+b+c==15)
//
// Canonical output order: (aa[10],aa[20],aa[30]) lexicographic ascending

`ifndef NUM_ITERATIONS
  `define NUM_ITERATIONS 10000
`endif

module distr_array_assoc_sum;
  class C;
    rand int aa[int];

    constraint c {
      aa[10] >= 0; aa[10] <= 7;
      aa[20] >= 0; aa[20] <= 7;
      aa[30] >= 0; aa[30] <= 7;
      aa[10] + aa[20] + aa[30] == 15;
    }

    function new();
      aa[10] = 0;
      aa[20] = 0;
      aa[30] = 0;
    endfunction
  endclass

  initial begin
    automatic C c = new;
    repeat (`NUM_ITERATIONS) begin
      if (!bit'(c.randomize())) $display("-UNSAT");
      else $display("%0d %0d %0d", c.aa[10], c.aa[20], c.aa[30]);
    end
    $finish;
  end
endmodule
