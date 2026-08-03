// Variables:
//   thold : bit [15:0] -- 16-bit unsigned
//   count : bit [15:0] -- 16-bit unsigned
//
// Constraints:
//   thold inside {0, [5:171]}
//   thold == 0 -> count == 0            (zero branch: 1 solution)
//   thold != 0 -> count inside {[thold-5:thold]}  (non-zero branch: 6 solutions each)
//
// Solutions: 1 + 167*6 = 1003


`ifndef NUM_ITERATIONS
  `define NUM_ITERATIONS 10000
`endif

module distr_ot_aon_wkup;
  class C;
    rand bit [15:0] thold;
    rand bit [15:0] count;

    constraint thold_c { thold inside {16'd0, [16'd5:16'd171]}; }
    constraint count_c {
      thold == 16'd0 -> count == 16'd0;
      thold != 16'd0 -> count inside {[thold - 16'd5 : thold]};
    }
  endclass

  initial begin
    automatic C c = new;
    repeat (`NUM_ITERATIONS) begin
      if (!bit'(c.randomize())) $display("-UNSAT");
      else $display("%0d %0d", c.thold, c.count);
    end
    $finish;
  end
endmodule
