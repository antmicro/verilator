// DESCRIPTION: Verilator: Missing hierarchical bind targets
// SPDX-FileCopyrightText: 2026 <TODO>
// SPDX-License-Identifier: CC0-1.0

module probe;
endmodule

module leaf;
endmodule

module t;
  leaf child ();
  bind t.missing probe root_probe ();
  bind child.missing probe local_probe ();
endmodule
