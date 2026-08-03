// Variables:
//   arr[3] : bit [2:0] -- three independent 3-bit elements, domain [0:7] each
//
// Constraint:
//   arr[0] < 5, arr[1] < 5, arr[2] < 5  (each element independently in [0:4])
//
// Solutions: 5 x 5 x 5 = 125


`ifndef NUM_ITERATIONS
  `define NUM_ITERATIONS 10000
`endif

module distr_array_basic;
  class C;
    rand bit [2:0] arr[3];

    constraint c {
      arr[0] < 3'd5;
      arr[1] < 3'd5;
      arr[2] < 3'd5;
    }
  endclass

  initial begin
    automatic C c = new;
    repeat (`NUM_ITERATIONS) begin
      if (!bit'(c.randomize())) $display("-UNSAT");
      else $display("%0d %0d %0d", c.arr[0], c.arr[1], c.arr[2]);
    end
    $finish;
  end
endmodule
