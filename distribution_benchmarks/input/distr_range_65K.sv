`ifndef NUM_ITERATIONS
  `define NUM_ITERATIONS 10000
`endif

module distr_range_999K;
  class C;
    rand bit [15:0] a;

    constraint ca { a inside {[16'd0:16'd65535]}; }
  endclass

  initial begin
    automatic C c = new;
    repeat (`NUM_ITERATIONS) begin
      if (!bit'(c.randomize())) $display("-UNSAT");
      else $display("%0d", c.a);
    end
    $finish;
  end
endmodule
