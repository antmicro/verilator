//// Solutions:
//   a = 0..1023  -> 1024 values
//   b = 1..1023  -> 1023 values
//   d is uniquely determined by d = a % b
//
// Total = 512 * 511 = 261632

`ifndef NUM_ITERATIONS
  `define NUM_ITERATIONS 10000
`endif

module distr_mod_1e9;
  class C;
    rand bit [8:0] a;
    rand bit [8:0] b;
    rand bit [8:0] d;

    constraint cb { b != 9'd0; }
    constraint cd { d == a % b; }
  endclass

  initial begin
    automatic C obj = new;
    repeat (`NUM_ITERATIONS) begin
      if (!bit'(obj.randomize())) $display("-UNSAT");
      else $display("%0d %0d", obj.a, obj.b);
    end
    $finish;
  end
endmodule
