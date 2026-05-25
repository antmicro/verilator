// DESCRIPTION: Verilator: FSM coverage ignores associative array selection operators
//
// This file ONLY is placed under the Creative Commons Public Domain.
// SPDX-FileCopyrightText: 2026 Wilson Snyder
// SPDX-License-Identifier: CC0-1.0

package P;
    typedef struct {
      logic [7:0] value[int];
    } C;
    typedef struct {
      C a; int b;
    } B;
    typedef struct {
      B a;
    } A;
endpackage
module t (
  input P::A a
);
  localparam int unsigned cmp = 'h3;
  assign b = (a.a.a.value[0] == cmp);

  initial begin
    $write("*-* All Finished *-*\n");
    $finish;
  end
endmodule
