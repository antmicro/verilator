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
  rand Middle m_mid;
  rand Middle m_mid_arr[2];

  function new();
    m_mid = new;
    m_mid.m_obj = new;
    foreach (m_mid.m_arr[i]) m_mid.m_arr[i] = new;
    foreach (m_mid_arr[i]) begin
      m_mid_arr[i] = new;
      m_mid_arr[i].m_obj = new;
      foreach (m_mid_arr[i].m_arr[j]) m_mid_arr[i].m_arr[j] = new;
    end
  endfunction

  // Case 6: Array elements member access in solve...before foreach loop
  constraint c_foreach {
    foreach (m_mid_arr[i]) {
      solve m_mid_arr[((i*2)/2)].m_obj.m_x before m_mid_arr[((i*2)/2) + 1].m_obj.m_x;

      m_mid_arr[((i*2)/2)].m_obj.m_x != m_mid_arr[((i*2)/2) + 1].m_obj.m_x;
    }
  }
endclass

module t_constraint_global_arr_unsup;
  initial begin
    automatic Outer o = new;
    if (o.randomize()) begin
      $display("*-* All Finished *-*");
    end
    else begin
      $display("*-* FAILED: randomize() returned 0 *-*");
      $stop;
    end
  end
endmodule
/* verilator lint_off WIDTHTRUNC */
