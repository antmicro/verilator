// DESCRIPTION: Verilator: SystemVerilog regression test
// SPDX-FileCopyrightText: 2026 The Verilator Authors
// SPDX-License-Identifier: CC0-1.0

package probes;
  int count;
endpackage

interface probe_if (input int data);
endinterface

module leaf #(parameter int Tag = 0) (input int data);
endmodule

module container;
  for (genvar i = 0; i < 4; i++) begin : gen_leaf
    leaf #(.Tag(i)) inst (.data(i));
  end
  leaf same[2:0] (.data(17));
endmodule

module t (
    input clk
);
  localparam int Pick = 3;
  container dut ();
  container sibling ();

  // The index is resolved in t; the port and parameter names are resolved in leaf.
  bind dut.gen_leaf[Pick].inst probe #(.Tag(Tag)) p (.data(data));
  bind dut.same[1] probe p (.*);
  virtual probe_if vif;

  always @(posedge clk) begin
    if (probes::count != 2) $fatal(1, "unexpected number of bound instances");
    if (dut.gen_leaf[3].inst.p.Tag != 3 || dut.gen_leaf[3].inst.p.data != 3) $fatal;
    vif = dut.gen_leaf[3].inst.p.backdoor;
    if (vif.data != 3 || dut.same[1].p.backdoor.data != 17) $fatal;
    $display("*-* All Finished *-*");
    $finish;
  end
endmodule
