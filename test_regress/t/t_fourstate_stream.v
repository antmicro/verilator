// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain.
// SPDX-FileCopyrightText: 2026 Antmicro
// SPDX-License-Identifier: CC0-1.0

`define stop $stop
`define checkb(gotv, expv) do if ((gotv) !== (expv)) begin $write("%%Error: %s:%0d:  got=%b exp=%b (%s !== %s)\n", `__FILE__, `__LINE__, (gotv), (expv), `"gotv`", `"expv`"); `stop; end while(0);

module t;
  function logic [3:0] f4(logic [3:0] x);
    return x;
  endfunction

  logic [3:0] got;

  initial begin
    `checkb({<<{f4(4'b10xz)}}, 4'bzx01);
    `checkb({<<2{f4(4'b10xz)}}, 4'bxz10);
    `checkb({<<3{f4(4'b10xz)}}, 4'b0xz1);
    `checkb({<<4{f4(4'b10xz)}}, 4'b10xz);

    `checkb({>>{f4(4'b10xz)}}, 4'b10xz);
    `checkb({>>2{f4(4'b10xz)}}, 4'b10xz);

    {<<{got}} = f4(4'b10xz);
    `checkb(got, 4'bzx01);
    {<<2{got}} = f4(4'b10xz);
    `checkb(got, 4'bxz10);
    {<<3{got}} = f4(4'b10xz);
    `checkb(got, 4'b0xz1);
    {<<4{got}} = f4(4'b10xz);
    `checkb(got, 4'b10xz);

    {>>{got}} = f4(4'b10xz);
    `checkb(got, 4'b10xz);
    {>>2{got}} = f4(4'b10xz);
    `checkb(got, 4'b10xz);

    $write("*-* All Finished *-*\n");
    $finish;
  end
endmodule
