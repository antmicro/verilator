// DESCRIPTION: Verilator: Randomize associative array elements added after construction
// SPDX-FileCopyrightText: 2026 The Verilator Authors
// SPDX-License-Identifier: CC0-1.0

// The solver must write back to the current keys, including when a key is
// replaced in pre_randomize and the element count stays the same.
class Packet;
  rand byte values[int];
  rand byte rows[2][int];
  rand bit flag;
  int key;
  byte expected;

  constraint entry {
    values[key] == expected;
    rows[1][key] == expected;
  }

  function void pre_randomize();
    values.delete();
    values[key] = 0;
    rows[1] = values;
  endfunction
endclass

module t;
  initial begin
    automatic Packet p = new;
    int ok;
    p.key = 3;
    p.expected = 17;
    ok = p.randomize();
    if (ok != 1 || p.values.num() != 1 || p.values[3] != 17) $stop;
    if (p.rows[1].num() != 1 || p.rows[1][3] != 17) $stop;

    p.key = 9;
    p.expected = 42;
    ok = p.randomize() with {flag == 1;};
    if (ok != 1 || p.values.num() != 1 || p.values.exists(3) || p.values[9] != 42) $stop;
    if (p.rows[1].num() != 1 || p.rows[1].exists(3) || p.rows[1][9] != 42 || !p.flag) $stop;

    $write("*-* All Finished *-*\n");
    $finish;
  end
endmodule
