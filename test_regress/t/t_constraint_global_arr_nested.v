// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain.
// SPDX-FileCopyrightText: 2025 PlanV GmbH
// SPDX-License-Identifier: CC0-1.0

/* verilator lint_off WIDTHTRUNC */
class Inner;
  rand int m_x;
  rand int m_y;
endclass

class Middle;
  rand Inner m_obj;
  rand Inner m_arr[3];
endclass

class Outer;
  int m_idx;
  rand Middle m_mid;
  rand Middle m_mid_arr[2];
  rand Middle m_mid_arr2[3][2];
  rand Middle m_mid_arr3[3][2];
  rand Inner m_assoc[int];
  rand Inner m_assoc_nested[int][bit];

  function new();
    m_idx = 1;
    m_mid = new;
    m_mid.m_obj = new;
    foreach (m_mid.m_arr[i]) m_mid.m_arr[i] = new;
    foreach (m_mid_arr[i]) begin
      m_mid_arr[i] = new;
      m_mid_arr[i].m_obj = new;
      foreach (m_mid_arr[i].m_arr[j]) m_mid_arr[i].m_arr[j] = new;
    end
    foreach (m_mid_arr2[i])
      foreach (m_mid_arr2[i][j]) begin
        m_mid_arr2[i][j] = new;
        m_mid_arr2[i][j].m_obj = new;
        foreach (m_mid_arr2[i][j].m_arr[k]) m_mid_arr2[i][j].m_arr[k] = new;
      end
    foreach (m_mid_arr3[i])
      foreach (m_mid_arr3[i][j]) begin
        m_mid_arr3[i][j] = new;
        m_mid_arr3[i][j].m_obj = new;
        foreach (m_mid_arr3[i][j].m_arr[k]) m_mid_arr3[i][j].m_arr[k] = new;
      end
    m_assoc[0] = new;
    m_assoc_nested[123][1] = new;
  endfunction

  // Case 1: Simple nested member access
  constraint c_simple {
    m_mid.m_obj.m_x == 100;
    m_mid.m_obj.m_y == 101;
  }

  // Case 2: Array indexing in the path
  constraint c_array_index {
    m_mid.m_arr[0].m_x == 200;
    m_mid.m_arr[0].m_y == 201;
  }

  constraint c_array_index_idx {
    m_mid.m_arr[m_idx].m_x == 202;
    m_mid.m_arr[m_idx].m_y == 203;
  }

  // Case 3: Nested array indexing
  constraint c_nested_array {
    m_mid_arr[0].m_obj.m_x == 300;
    m_mid_arr[0].m_obj.m_y == 301;
  }

  // Case 4: Multiple array indices
  constraint c_multi_array {
    m_mid_arr[1].m_arr[2].m_y == 400;
  }

  // Case 5: Associative array element member access
  constraint c_assoc {
    m_assoc[0].m_x == 500;
  }

  constraint c_assoc_nested {
    m_assoc_nested[123][1].m_x == 501;
  }

  // Case 6: foreach
  constraint c_foreach {
    foreach (m_mid_arr2[i, j])
      m_mid_arr2[i][j].m_obj.m_x == i + j;
  }
  constraint c_foreach2 {
    foreach (m_mid_arr3[i])
      foreach (m_mid_arr3[i][j])
        m_mid_arr3[i][j].m_obj.m_x == i - j;
  }
endclass

module t_constraint_global_arr_unsup;
  initial begin
    automatic Outer o = new;
    if (o.randomize()) begin
      $display("Case 1 - Simple: mid.obj.x = %0d (expected 100)", o.m_mid.m_obj.m_x);
      $display("Case 1 - Simple: mid.obj.y = %0d (expected 101)", o.m_mid.m_obj.m_y);
      $display("Case 2 - Array[0]: mid.arr[0].x = %0d (expected 200)", o.m_mid.m_arr[0].m_x);
      $display("Case 2 - Array[0]: mid.arr[0].y = %0d (expected 201)", o.m_mid.m_arr[0].m_y);
      $display("Case 3 - Nested[0]: mid_arr[0].obj.x = %0d (expected 300)", o.m_mid_arr[0].m_obj.m_x);
      $display("Case 3 - Nested[0]: mid_arr[0].obj.y = %0d (expected 301)", o.m_mid_arr[0].m_obj.m_y);
      $display("Case 4 - Multi[1][2]: mid_arr[1].arr[2].y = %0d (expected 400)", o.m_mid_arr[1].m_arr[2].m_y);

      // Check results
      foreach (o.m_mid_arr2[i])
        foreach (o.m_mid_arr2[i][j])
          if (o.m_mid_arr2[i][j].m_obj.m_x != i + j) $stop;
      foreach (o.m_mid_arr3[i])
        foreach (o.m_mid_arr3[i][j])
          if (o.m_mid_arr3[i][j].m_obj.m_x != i - j) $stop;
      if (o.m_mid.m_obj.m_x == 100 && o.m_mid.m_obj.m_y == 101 &&
          o.m_mid.m_arr[0].m_x == 200 && o.m_mid.m_arr[0].m_y == 201 &&
          o.m_mid.m_arr[1].m_x == 202 && o.m_mid.m_arr[1].m_y == 203 &&
          o.m_mid_arr[0].m_obj.m_x == 300 && o.m_mid_arr[0].m_obj.m_y == 301 &&
          o.m_mid_arr[1].m_arr[2].m_y == 400 &&
          o.m_assoc[0].m_x == 500 && o.m_assoc_nested[123][1].m_x == 501) begin
        $display("*-* All Finished *-*");
        $finish;
      end
      else begin
        $display("*-* FAILED *-*");
        $stop;
      end
    end
    else begin
      $display("*-* FAILED: randomize() returned 0 *-*");
      $stop;
    end
  end
endmodule
/* verilator lint_off WIDTHTRUNC */
