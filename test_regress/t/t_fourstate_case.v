// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain.
// SPDX-FileCopyrightText: 2026 Antmicro
// SPDX-License-Identifier: CC0-1.0

module t;
  logic [2:0] x;
  initial begin
    case (x)
      0: $stop;
      1, 2: $stop;
      default: ;
    endcase
    x = 0;
    case (x)
      0: ;
      1, 2: $stop;
      default: $stop;
    endcase
    x = 1;
    case (x)
      0: $stop;
      1, 2: ;
      default: $stop;
    endcase
    x = 2;
    case (x)
      0: $stop;
      1, 2: ;
      default: $stop;
    endcase
    x = 3;
    case (x)
      0: $stop;
      1, 2: $stop;
      default: ;
    endcase
    x = 3'bxz1;
    casex (x)
      0: $stop;
      7: ;
      default: $stop;
    endcase
    casex (x)
      0: $stop;
      default: ;
    endcase
    casez (x)
      0: $stop;
      7: $stop;
      default: ;
    endcase
    x = 3'b1z1;
    casez (x)
      0: $stop;
      3: $stop;
      3'b111: ;
      default: $stop;
    endcase
    $write("*-* All Finished *-*\n");
    $finish;
  end
endmodule
