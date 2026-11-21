// DESCRIPTION: A module item following a named fork remains in module scope.
// SPDX-License-Identifier: CC0-1.0

module worker(input bit start);
  bit a, b;
  always @(posedge start) fork : jobs
    #1 a = 1;
    #2 b = 1;
  join
  bit visible = 1;
endmodule

module t;
  bit start;
  worker dut(start);
  initial begin
    #1 start = 1;
    #4;
    if (!dut.visible || !dut.a || !dut.b) $stop;
    $write("*-* All Finished *-*\n");
    $finish;
  end
endmodule
