// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain
// SPDX-FileCopyrightText: 2026 Antmicro
// SPDX-License-Identifier: CC0-1.0

package P;
  typedef struct packed{
    logic [7:0] vs;
  } C;
  typedef struct packed{
    C a; int b;
  } B;
  typedef struct packed{
    B a;
  } A;
endpackage
module t (
  input P::A a,
  output logic b,
  output logic c,
  output logic d
);
  logic [7:0] va[int];
  logic [7:0] va2d[int][int];

  assign b = (a.a.a.vs == 8'h0);
  assign c = (va[0] == 8'h0);
  assign d = (va2d[0][0] == 8'h0);

  initial begin
    $write("*-* All Finished *-*\n");
    $finish;
  end
endmodule
