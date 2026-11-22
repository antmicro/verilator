// DESCRIPTION: Verilator: Select an interface instance array through a virtual interface
//
// This file ONLY is placed under the Creative Commons Public Domain.
// SPDX-FileCopyrightText: 2026 The Verilator Authors
// SPDX-License-Identifier: CC0-1.0

// verilog_format: off
`define stop $stop
`define checkd(gotv,expv) do if ((gotv) !== (expv)) begin $write("%%Error: %s:%0d: got=%0d exp=%0d\n", `__FILE__, `__LINE__, (gotv), (expv)); `stop; end while(0);
// verilog_format: on

// Selecting an element of a nested interface instance array through a virtual
// interface must follow both the declared array indices and the current handle.
interface leaf_if;
  int value;
endinterface

interface child_if;
  leaf_if leaf ();
  bit [10:0] packed_data;
  int unpacked_data[5];
endinterface

interface parent_if;
  typedef bit [10:0] data_t;
  child_if children[2:1] ();
  child_if negative[-1:-2] ();
  data_t packed_data;
  int unpacked_data[2:6];
  modport data_mp(inout packed_data, unpacked_data);
endinterface

module t;
  parent_if a ();
  parent_if b ();
  virtual parent_if vif;
  virtual parent_if parents[2];
  int calls;

  function automatic virtual parent_if selected(int index);
    ++calls;
    return parents[index];
  endfunction
  virtual child_if child;
  virtual parent_if.data_mp data_vif;

  initial begin
    a.children[1].leaf.value = 11;
    a.children[2].leaf.value = 12;
    b.children[2].leaf.value = 22;
    vif = a;
    `checkd(vif.children[1].leaf.value, 11);
    child = vif.children[2];
    `checkd(child.leaf.value, 12);
    vif = b;
    `checkd(vif.children[2].leaf.value, 22);
    `checkd(child.leaf.value, 12);

    parents[0] = a;
    parents[1] = b;
    foreach (parents[i]) begin
      child = parents[i].children[2];
      `checkd(child.leaf.value, (i == 0 ? 12 : 22));
      child = selected(i).children[2];
      `checkd(child.leaf.value, (i == 0 ? 12 : 22));
    end
    b.negative[-1].leaf.value = 31;
    child = selected(1).negative[-1];
    `checkd(child.leaf.value, 31);
    `checkd(calls, 3);

    data_vif = b;

    // Data selected through a virtual interface permits runtime array indexes
    // (IEEE 1800-2023 25.9), including through nested interfaces and modports.
    for (int i = 0; i < 5; ++i) begin
      vif.packed_data[i] = 1'b1;
      vif.unpacked_data[i + 2] = 20 + i;
      vif.children[2].packed_data[i] = 1'b1;
      vif.children[2].unpacked_data[i] = 40 + i;
    end
    for (int i = 0; i < 5; ++i) begin
      `checkd(vif.packed_data[i], 1'b1);
      `checkd(data_vif.packed_data[i], 1'b1);
      `checkd(data_vif.unpacked_data[i + 2], 20 + i);
      `checkd(vif.unpacked_data[i + 2], 20 + i);
      `checkd(vif.children[2].packed_data[i], 1'b1);
      `checkd(vif.children[2].unpacked_data[i], 40 + i);
    end
    `checkd(b.packed_data, 11'h1f);
    `checkd(a.packed_data, 11'h0);
    `checkd(b.children[2].packed_data, 11'h1f);
    `checkd(a.children[2].packed_data, 11'h0);

    $write("*-* All Finished *-*\n");
    $finish;
  end
endmodule
