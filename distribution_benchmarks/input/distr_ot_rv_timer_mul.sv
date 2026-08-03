// Inspired by OpenTitan:
//   hw/ip/rv_timer/dv/env/seq_lib/rv_timer_random_vseq.sv
//
// Models the timer tick/prescale relationship from rv_timer:
//   the number of clock cycles until expiry = ticks * (prescale + 1)
//   must not exceed a maximum budget.
//
// Simplified to scalar variables (original uses per-hart arrays).
// solve...before removed (original uses it for ordering only).
//
// Variables:
//   prescale : bit [13:0] -- prescaler value, constrained to [0:9]
//   ticks    : bit [13:0] -- tick count, constrained to [1:1000]
//
// Both declared 14-bit to keep arithmetic in a consistent width context
// and avoid WIDTHEXPAND in the multiplication expression.
//
// Constraint:
//   ticks * (prescale + 1) <= 1000
//
// No overflow: max product = 1000 * 10 = 10000 < 2^14 = 16384
//
// Solutions: 2927 (x,y) pairs with x in [0:9], y in [1:1000], y*(x+1) <= 1000
//   prescale=0: 1000 pairs, prescale=1: 500, ..., prescale=9: 100
//
// Canonical output order: (prescale, ticks) lexicographic ascending

`ifndef NUM_ITERATIONS
  `define NUM_ITERATIONS 10000
`endif

module distr_ot_rv_timer_mul;
  class C;
    rand bit [13:0] prescale;
    rand bit [13:0] ticks;

    constraint prescale_c { prescale inside {[0:9]}; }
    constraint ticks_c    { ticks inside {[1:1000]}; }
    constraint mul_c      { ticks * (prescale + 14'd1) <= 14'd1000; }
  endclass

  initial begin
    automatic C c = new;
    repeat (`NUM_ITERATIONS) begin
      if (!bit'(c.randomize())) $display("-UNSAT");
      else $display("%0d %0d", c.prescale, c.ticks);
    end
    $finish;
  end
endmodule
