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
