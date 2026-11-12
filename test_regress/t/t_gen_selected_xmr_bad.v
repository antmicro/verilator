// DESCRIPTION: Verilator: Conditional generate hierarchy resolution
// SPDX-FileCopyrightText: 2026 The Verilator Authors
// SPDX-License-Identifier: CC0-1.0

module harness #(parameter bit Check = 0);
  if (Check) begin : access
    initial absent_target.run_check();
  end
endmodule

module t;
  harness #(.Check(1)) h ();
endmodule
