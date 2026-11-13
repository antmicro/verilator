// DESCRIPTION: Check that typed bind lists instrument exactly the named instances in their own scopes.
// SPDX-FileCopyrightText: 2026 The Verilator Authors
// SPDX-License-Identifier: CC0-1.0

package counts;
  int hits[7];
endpackage

module observer (
    input int id
);
  initial begin
    #1;
    counts::hits[id]++;
  end
endmodule

module leaf (
    input int id
);
endmodule

module branch (
    input int first,
    second
);
  leaf a (first);
  leaf b (second);
endmodule

module other_leaf (
    input int id
);
endmodule

module selectable #(
    parameter bit UseLeaf = 1
);
  if (UseLeaf) begin : choice
    leaf pick[1:0] ({32'd6, 32'd5});
  end
  else begin : choice
    other_leaf pick[1:0] ({32'd6, 32'd5});
  end
endmodule

module t;
  branch left (
      0,
      1
  );
  branch right (
      2,
      3
  );
  leaf spare (4);
  selectable selected ();
  selectable #(.UseLeaf(0)) other ();

  bind leaf : t.left.a, right.b observer multi (.id(id));
  bind leaf : left.b observer single_target (.*);
  // The generate arms have the same hierarchy but different leaf types.
  bind leaf : selected.choice.pick[1] observer indexed (.*);

  initial begin
    #2;
    foreach (counts::hits[i]) begin
      if (counts::hits[i] != int'(i inside {0, 1, 3, 6}))
        $fatal(1, "Instance %0d got %0d probes", i, counts::hits[i]);
    end
    $display("*-* All Finished *-*");
    $finish;
  end
endmodule
