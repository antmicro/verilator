// DESCRIPTION: Check read, write and delete access on associative arrays captured by a fork.
//
// This file ONLY is placed under the Creative Commons Public Domain.
// SPDX-FileCopyrightText: 2026 The Verilator Authors
// SPDX-License-Identifier: CC0-1.0

module t;
  task automatic check_array;
    int entries[int][int];
    fork
      begin
        #1;
        entries[2][3] = 12;
        entries[2][4] = 34;
      end
    join_none
    #2;
    if (entries.num() != 1 || entries[2].num() != 2) $stop;
    if (entries[2][3] != 12 || entries[2][4] != 34) $stop;
    if (entries[9][3] != 0) $stop;
    if (entries.exists(9)) $stop;
    entries[2].delete(3);
    if (entries[2].exists(3) || entries[2][4] != 34) $stop;
  endtask

  initial begin
    check_array();
    $write("*-* All Finished *-*\n");
    $finish;
  end
endmodule
