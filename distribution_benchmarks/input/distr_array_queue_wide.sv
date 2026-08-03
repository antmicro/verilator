// Variables:
//   q[4] : bit [7:0] queue elements -- four 8-bit-stored elements,
//          domain restricted to [0:7] each
//
// Constraint:
//   q[i] < 8 for all i
//
// Solutions: 8^4 = 4096

`ifndef NUM_ITERATIONS
  `define NUM_ITERATIONS 10000
`endif

module distr_array_queue_wide;
  class C;
    rand bit [7:0] q[$];

    constraint c {
      foreach (q[i]) { q[i] < 8'd8; }
    }

    function new();
      q = {};
      for (int i = 0; i < 4; i++) q.push_back(8'h0);
    endfunction
  endclass

  initial begin
    automatic C c = new;
    repeat (`NUM_ITERATIONS) begin
      if (!bit'(c.randomize())) $display("-UNSAT");
      else $display("%0d %0d %0d %0d", c.q[0], c.q[1], c.q[2], c.q[3]);
    end
    $finish;
  end
endmodule
