// DESCRIPTION: Verilator: SystemVerilog regression test
// SPDX-FileCopyrightText: 2026 The Verilator Authors
// SPDX-License-Identifier: CC0-1.0

interface access_if;
  task automatic read_value(output int value);
    parent_read(value);
  endtask
endinterface
module container #(parameter int Tag = 0);
  access_if a ();
  task automatic parent_read(output int value);
    value = Tag;
  endtask
endmodule
module different_parent;
  access_if a ();
  task automatic parent_read(output int value);
    value = 42;
  endtask
endmodule
module t;
  container #(.Tag(1)) first ();
  container #(.Tag(2)) second ();
  different_parent third ();
  initial begin
    int value;
    first.a.read_value(value);
    if (value != 1) $fatal(1, "wrong first parent");
    second.a.read_value(value);
    if (value != 2) $fatal(1, "wrong second parent");
    third.a.read_value(value);
    if (value != 42) $fatal(1, "wrong parent module");
    $display("*-* All Finished *-*");
    $finish;
  end
endmodule
