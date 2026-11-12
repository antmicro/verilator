// DESCRIPTION: Test for IEEE 1800-2023 6.22.2 - valid array assignments with matching state types
//
// This file ONLY is placed under the Creative Commons Public Domain.
// SPDX-FileCopyrightText: 2026 Wilson Snyder
// SPDX-License-Identifier: CC0-1.0

module t;
  typedef int fixed_t[2];
  int values[$];
  fixed_t fixed_values;
  int dynamic_values[];

  function automatic void update_queue(inout int a[$]);
    if (a.size() != 2 || a[0] != 11 || a[1] != 23) $stop;
    a = '{31, 47};
  endfunction

  task automatic update_fixed(inout fixed_t a);
    if (a[0] != 31 || a[1] != 47) $stop;
    a = '{11, 23};
  endtask

  initial begin
    fixed_values = '{11, 23};
    update_queue(fixed_values);
    if (fixed_values[0] != 31 || fixed_values[1] != 47) $stop;

    values = '{31, 47};
    update_fixed(values);
    if (values.size() != 2 || values[0] != 11 || values[1] != 23) $stop;

    dynamic_values = '{31, 47};
    update_fixed(dynamic_values);
    if (dynamic_values.size() != 2 || dynamic_values[0] != 11 || dynamic_values[1] != 23) $stop;
    $write("*-* All Finished *-*\n");
    $finish;
  end
endmodule
