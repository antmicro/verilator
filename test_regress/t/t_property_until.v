// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain.
// SPDX-FileCopyrightText: 2026 Antmicro
// SPDX-License-Identifier: CC0-1.0

`define stop $stop
`define checkh(gotv,
               expv) do if ((gotv) !== (expv)) begin $write("%%Error: %s:%0d:  got='h%p exp='h%p\n", `__FILE__,`__LINE__, (gotv), (expv)); `stop; end while(0)

module t (  /*AUTOARG*/
    // Inputs
    clk
);

  input clk;

  typedef struct {
    int fails;
    int passs;
  } result_t;

  // TODO: make arrays mt-safe and use them instead of separate variables
  result_t result1;
  result_t result2;
  result_t result3;
  result_t result4;
  result_t result5;
  result_t result6;
  result_t result7;
  result_t result8;
  result_t result9;
  result_t result10;
  result_t result11;
  result_t result12;
  result_t result13;

  result_t expected1;
  result_t expected2;
  result_t expected3;
  result_t expected4;
  result_t expected5;
  result_t expected6;
  result_t expected7;
  result_t expected8;
  result_t expected9;
  result_t expected10;
  result_t expected11;
  result_t expected12;
  result_t expected13;

  localparam MAX = 15;
  integer cyc = 1;

  assert property (@(posedge clk) 0 until 1)
    result1.passs++;
  else result1.fails++;

  assert property (@(posedge clk) 1 until 0)
    result2.passs++;
  else result2.fails++;

  assert property (@(posedge clk) cyc < 5 until cyc >= 5)
    result3.passs++;
  else result3.fails++;

  assert property (@(posedge clk) cyc % 3 == 0 until cyc % 5 == 0)
    result4.passs++;
  else result4.fails++;

  assert property (@(posedge clk) cyc % 3 != 0 until_with cyc % 4 != 0)
    result5.passs++;
  else result5.fails++;

  assert property (@(posedge clk) 0 s_until 1)
    result6.passs++;
  else result6.fails++;

  assert property (@(posedge clk) cyc < 5 s_until cyc >= 5)
    result7.passs++;
  else result7.fails++;

  assert property (@(posedge clk) cyc % 3 == 0 s_until cyc % 5 == 0)
    result8.passs++;
  else result8.fails++;

  // Check that s_until accepts immediately when RHS is true, even if LHS is false.
  assert property (@(posedge clk) cyc % 2 == 0 s_until 1)
    result9.passs++;
  else result9.fails++;

  // Check that s_until_with requires LHS when RHS is true on the same tick.
  assert property (@(posedge clk) 0 s_until_with 1)
    result10.passs++;
  else result10.fails++;

  assert property (@(posedge clk) 1 s_until_with cyc >= 5)
    result11.passs++;
  else result11.fails++;

  assert property (@(posedge clk) cyc <= 5 s_until_with cyc >= 5)
    result12.passs++;
  else result12.fails++;

  assert property (@(posedge clk) cyc < 5 s_until_with cyc >= 5)
    result13.passs++;
  else result13.fails++;

  always @(edge clk) begin
    ++cyc;
    if (cyc == MAX) begin
      expected1 = '{0, 7};
      // expected2 shouldn't be initialized
      expected3 = '{0, 7};
      expected4 = '{5, 2};
      expected5 = '{2, 5};
      expected6 = '{0, 7};
      expected7 = '{0, 7};
      expected8 = '{5, 2};
      expected9 = '{0, 7};
      expected10 = '{7, 0};
      expected11 = '{0, 7};
      expected12 = '{4, 3};
      expected13 = '{7, 0};

      `checkh(result1, expected1);
      `checkh(result2, expected2);
      `checkh(result3, expected3);
      `checkh(result4, expected4);
      `checkh(result5, expected5);
      `checkh(result6, expected6);
      `checkh(result7, expected7);
      `checkh(result8, expected8);
      `checkh(result9, expected9);
      `checkh(result10, expected10);
      `checkh(result11, expected11);
      `checkh(result12, expected12);
      `checkh(result13, expected13);

      $write("*-* All Finished *-*\n");
      $finish;
    end
  end
endmodule
