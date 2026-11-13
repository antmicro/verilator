// DESCRIPTION: Verilator: SystemVerilog regression test
// SPDX-FileCopyrightText: 2026 The Verilator Authors
// SPDX-License-Identifier: CC0-1.0

module probe #(parameter int Tag = 0) (input int data);
  probe_if backdoor (.data(data));
  initial probes::count++;
endmodule
