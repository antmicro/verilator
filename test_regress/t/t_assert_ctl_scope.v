// DESCRIPTION: Verilator: Verilog Test module
// SPDX-License-Identifier: CC0-1.0

module child;
  check_child: assert property (@(posedge $root.t.clk) 1);
endmodule

module t;
  bit clk;
  child a();
  child b();
  int count;
  function automatic bit checked();
    ++count;
    return 1;
  endfunction
  initial begin
    $assertoff(0, a.check_child);
    $assertcontrol(4, 15, 7, 0, a, b);
    $assertoff(1);
    assert (checked());
    if (count != 1) $stop;
    $assertoff(0);
    assert (checked());
    if (count != 1) $stop;
    $asserton;
    assert (checked());
    if (count != 2) $stop;
    $display("*-* All Finished *-*");
    $finish;
  end
endmodule
