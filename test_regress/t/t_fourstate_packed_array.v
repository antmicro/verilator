// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain.
// SPDX-FileCopyrightText: 2026 Antmicro
// SPDX-License-Identifier: CC0-1.0

`define stop $stop
`define checkb(gotv, expv) do if ((gotv) !== (expv)) begin $write("%%Error: %s:%0d:  got=%b exp=%b (%s !== %s)\n", `__FILE__, `__LINE__, (gotv), (expv), `"gotv`", `"expv`"); `stop; end while(0);

module t;
  typedef logic [1:0][3:0] arr2_t;
  typedef logic [1:0][1:0][3:0] arr3_t;

  function arr2_t f2(arr2_t x);
    return x;
  endfunction

  function arr3_t f3(arr3_t x);
    return x;
  endfunction

  arr2_t arr2;
  arr3_t arr3;
  int idx;
  int outer;
  int inner;

  initial begin
    arr2 = f2(8'b10xz_0z1x);
    `checkb(arr2, 8'b10xz_0z1x);
    `checkb(arr2[1], 4'b10xz);
    `checkb(arr2[0], 4'b0z1x);

    idx = 1;
    `checkb(arr2[idx], 4'b10xz);
    idx = 0;
    arr2[idx] = f2(8'b0000_zx01)[0];
    `checkb(arr2, 8'b10xz_zx01);

    arr3 = f3(16'b10xz_0z1x_zx01_xz10);
    `checkb(arr3, 16'b10xz_0z1x_zx01_xz10);
    `checkb(arr3[1][1], 4'b10xz);
    `checkb(arr3[1][0], 4'b0z1x);
    `checkb(arr3[0][1], 4'bzx01);
    `checkb(arr3[0][0], 4'bxz10);

    outer = 0;
    inner = 1;
    `checkb(arr3[outer][inner], 4'bzx01);
    arr3[outer][inner] = f2(8'b1111_0xz1)[0];
    `checkb(arr3, 16'b10xz_0z1x_0xz1_xz10);

    $write("*-* All Finished *-*\n");
    $finish;
  end
endmodule
