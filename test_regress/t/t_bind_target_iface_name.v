// DESCRIPTION: Verilator: Bind same-named interfaces in distinct instances
// SPDX-FileCopyrightText: 2026 The Verilator Authors
// SPDX-License-Identifier: CC0-1.0

interface observer #(parameter int Tag = 42);
endinterface
module leaf #(parameter int Value = 0);
endmodule
module t;
  leaf #(.Value(1)) a();
  leaf #(.Value(2)) b();
  bind a observer #(.Tag(Value)) probe();
  bind b observer #(.Tag(Value)) probe();
  initial begin
    if (a.probe.Tag != 1 || b.probe.Tag != 2) $fatal;
    $display("*-* All Finished *-*");
    $finish;
  end
endmodule
