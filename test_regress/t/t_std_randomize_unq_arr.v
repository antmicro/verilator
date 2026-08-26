// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain
// SPDX-FileCopyrightText: 2026 Antmicro
// SPDX-License-Identifier: CC0-1.0

// std::randomize with unique constraint on static array
class StaticArr;
  int arr[10];
  int prev_arr[10];
  int ok[10];

  function void test();
    if ((std::randomize(arr) with {
      unique {arr};
      foreach (arr[i]) {
        arr[i] <= 10;
        arr[i] >= 1;
      }
    }) != 1) $stop;
    foreach (arr[i]) begin
      foreach (arr[j]) begin
        if (i == j) continue;
        if (arr[i] == arr[j]) $stop;
      end
      if (arr[i] > 10 || arr[i] < 1) $stop;
      if (arr[i] != prev_arr[i]) ok[i] = 1;
      prev_arr[i] = arr[i];
    end

    repeat (10) begin
      if ((std::randomize(arr) with {
        unique {arr};
        foreach (arr[i]) {
          arr[i] <= 10;
          arr[i] >= 1;
        }
      }) != 1) $stop;

      foreach (arr[i]) begin
        foreach (arr[j]) begin
          if (i == j) continue;
          if (arr[i] == arr[j]) $stop;
        end
        if (arr[i] > 10 || arr[i] < 1) $stop;
        if (arr[i] != prev_arr[i]) ok[i] = 1;
        prev_arr[i] = arr[i];
      end
      foreach (ok[i]) if (ok[i] != 1) $stop;
    end
  endfunction
endclass

// std::randomize with unique constraint on dynamic array
class DynArr;
  int arr[];
  int prev_arr[10];
  int ok[10];

  function void test();
    arr = new[10];
    if ((std::randomize(arr) with {
      unique {arr};
      foreach (arr[i]) {
        arr[i] <= 10;
        arr[i] >= 1;
      }
    }) != 1) $stop;
    foreach (arr[i]) begin
      foreach (arr[j]) begin
        if (i == j) continue;
        if (arr[i] == arr[j]) $stop;
      end
      if (arr[i] > 10 || arr[i] < 1) $stop;
      if (arr[i] != prev_arr[i]) ok[i] = 1;
      prev_arr[i] = arr[i];
    end

    repeat (10) begin
      if ((std::randomize(arr) with {
        unique {arr};
        foreach (arr[i]) {
          arr[i] <= 10;
          arr[i] >= 1;
        }
      }) != 1) $stop;

      foreach (arr[i]) begin
        foreach (arr[j]) begin
          if (i == j) continue;
          if (arr[i] == arr[j]) $stop;
        end
        if (arr[i] > 10 || arr[i] < 1) $stop;
        if (arr[i] != prev_arr[i]) ok[i] = 1;
        prev_arr[i] = arr[i];
      end
      foreach (ok[i]) if (ok[i] != 1) $stop;
    end
  endfunction
endclass

module t;
  StaticArr stat;
  DynArr dyn;
  initial begin
    stat = new;
    stat.test();

    dyn = new;
    dyn.test();

    $write("*-* All Finished *-*\n");
    $finish;
  end
endmodule
