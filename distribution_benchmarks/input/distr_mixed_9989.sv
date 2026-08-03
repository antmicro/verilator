// Mixed constraint benchmark: arithmetic + bitwise implication, ~10k solutions.
// Models a bounded address/offset pair where high-address accesses are
// restricted to small offsets.
//
// Variables:
//   base  : bit [7:0] -- 8-bit unsigned, domain [0:255]
//   delta : bit [7:0] -- 8-bit unsigned, domain [0:255]
//
// Constraints:
//   base  inside {[0:199]}
//   delta inside {[1:63]}
//   base + delta <= 200                  (arithmetic bound)
//   base[7] -> delta inside {[1:35]}     (bitwise implication: if base >= 128, delta is small)
//
// Solutions: 9989
//   base in [0:127]:  delta in [1:63] subject to base+delta<=200 -> 9314 solutions
//   base in [128:199]: delta in [1:35] subject to base+delta<=200 -> 675 solutions
//
// No overflow: base+delta <= 262 < 2^32 (comparison done in 32-bit context)
//
// Canonical output order: (base, delta) lexicographic ascending

`ifndef NUM_ITERATIONS
  `define NUM_ITERATIONS 10000
`endif

module distr_mixed_9989;
  class C;
    rand bit [7:0] base;
    rand bit [7:0] delta;

    constraint base_c  { base  inside {[8'd0:8'd199]}; }
    constraint delta_c { delta inside {[8'd1:8'd63]}; }
    constraint sum_c   { base + delta <= 200; }
    constraint high_c  { base[7] -> delta inside {[8'd1:8'd35]}; }
  endclass

  initial begin
    automatic C c = new;
    repeat (`NUM_ITERATIONS) begin
      if (!bit'(c.randomize())) $display("-UNSAT");
      else $display("%0d %0d", c.base, c.delta);
    end
    $finish;
  end
endmodule
