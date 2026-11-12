// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain.
// SPDX-FileCopyrightText: 2026 <TODO>
// SPDX-License-Identifier: CC0-1.0

// verilog_format: off
`define stop $stop
`define checkd(gotv,expv) do if ((gotv) !== (expv)) begin \
   $write("%%Error: %s:%0d:  got=%0d exp=%0d\n", `__FILE__, `__LINE__, (gotv), (expv)); `stop; end while(0)
// verilog_format: on

class Item;
  rand bit [3:0] x;
  rand bit gate;
  rand bit [3:0] a[3];
  bit pick;
  bit weight;
  bit [3:0] wanted;
  constraint c {
    if (gate) {
      if (pick) {
        x dist {
          [1 : 4] :/ 9,
          [8 : 10] :/ 1,
          10 := 0,
          12 := weight
        };
        x != 3;
      } else {
        x dist {[12 : 14] :/ 1};
      }
      foreach (a[i]) {(i != 1) -> a[i] dist {[1 : 5] :/ 1};}
    } else {
      x == 0;
    }
    x == wanted;
  }
endclass

module t;
  initial begin
    Item obj;
    int ok;
    bit legal;
    obj = new;
    for (int p = 0; p < 2; ++p) begin
      obj.pick = 1'(p);
      for (int i = 0; i < 16; ++i) begin
        obj.wanted = 4'(i);
        legal = obj.pick ? (i inside {1, 2, 4, [8 : 10]}) : (i inside {[12 : 14]});
        ok = obj.randomize() with {
          gate == 1;
          a[0] == 2;
          a[1] == 15;
          a[2] == 4;
        };
        if ((ok == 1) != legal) $fatal(1, "pick=%0d x=%0d expected SAT=%0d", p, i, legal);
        if (ok == 1) begin
          `checkd(obj.x, obj.wanted);
          `checkd(obj.a[0], 2);
          `checkd(obj.a[1], 15);
          `checkd(obj.a[2], 4);
        end
      end
    end
    obj.pick = 1;
    obj.weight = 1;
    obj.wanted = 12;
    ok = obj.randomize() with {gate == 1;};
    `checkd(ok, 1);
    `checkd(obj.x, 12);
    obj.wanted = 0;
    ok = obj.randomize() with {
      gate == 0;
      a[0] == 15;
    };
    `checkd(ok, 1);
    `checkd(obj.x, 0);
    `checkd(obj.a[0], 15);
    $write("*-* All Finished *-*\n");
    $finish;
  end
endmodule
