// DESCRIPTION: Verilator: Bind through directly nested conditional generate alternatives
// SPDX-FileCopyrightText: 2026 The Verilator Authors
// SPDX-License-Identifier: CC0-1.0

module leaf;
endmodule

module probe;
  localparam bit Present = 1;
endmodule

module branches #(parameter int Select = 1);
  if (Select == 0) begin : first
    leaf l();
  end else if (Select == 1) begin : selected
    leaf l();
  end else begin : selected
    leaf l();
  end
endmodule

module t;
  branches dut();
  branches #(.Select(2)) other();
  bind dut.selected.l probe p();
  bind other.selected.l probe p();

  initial begin
    if (!dut.selected.l.p.Present || !other.selected.l.p.Present) $fatal;
    $display("*-* All Finished *-*");
    $finish;
  end
endmodule
