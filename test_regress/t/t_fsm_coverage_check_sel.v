// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain
// SPDX-FileCopyrightText: 2026 Antmicro
// SPDX-License-Identifier: CC0-1.0

package P;
  typedef struct packed{
    logic [7:0] value;
  } C;
  typedef struct packed{
    C a; int b;
  } B;
  typedef struct packed{
    B a;
  } A;
endpackage
module t (
  input P::A a
);
  localparam int unsigned cmp = 'h3;
  assign recovery_mode = (a.a.a.value == cmp);

  initial begin
    $write("*-* All Finished *-*\n");
    $finish;
  end
endmodule
