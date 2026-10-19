// DESCRIPTION: Verilator: Assertion processes and sampling history follow the class owner
//
// This file ONLY is placed under the Creative Commons Public Domain.
// SPDX-FileCopyrightText: 2026 Wilson Snyder
// SPDX-License-Identifier: CC0-1.0

interface assertion_owner (
    input bit clk
);
  int value;
  default clocking cb @(posedge clk);
  endclocking
  class Reader;
    function int history();
      return $past(value, 2);
    endfunction
    task check_prop();
      assert property (value > 0 |-> ##1 value > 0);
    endtask
  endclass
  Reader first, second;
endinterface

module t;
  bit clk_first, clk_second;
  always #5 clk_first = ~clk_first;
  always #7 clk_second = ~clk_second;
  assertion_owner first (clk_first), second (clk_second);
  initial begin
    first.value  = 11;
    second.value = 29;
    // Sampling starts with the owner, before any Reader is constructed.
    #40;
    first.first  = new;
    first.second = new;
    second.first = new;
    if (first.first.history() != 11 || first.second.history() != 11) $stop;
    if (second.first.history() != 29) $stop;
    first.first.check_prop();
    second.first.check_prop();
    first.value  = 31;
    second.value = 47;
    #20;
    // Only the first owner has had two sampling edges since the value change.
    if (first.first.history() != 31 || second.first.history() != 29) $stop;
    #20;
    if (first.first.history() != 31 || second.first.history() != 47) $stop;
    first.second  = new;
    second.second = new;
    if (first.second.history() != 31 || second.second.history() != 47) $stop;
    first.second.check_prop();
    second.second.check_prop();
    $write("*-* All Finished *-*\n");
    $finish;
  end
  initial begin
    #1000;
    $stop;
  end
endmodule
