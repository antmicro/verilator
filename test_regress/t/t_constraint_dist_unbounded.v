// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain.
// SPDX-FileCopyrightText: 2026 <TODO>
// SPDX-License-Identifier: CC0-1.0

class Packet;
  rand bit [3:0] offset;
  int limit;
  constraint pick {
    offset dist {[0:limit] :/ 3, [(limit + 1):$] :/ 1};
  }
endclass

class SignedPacket;
  rand bit signed [3:0] offset;
  constraint pick {
    offset dist {[$:-2] := 1, [6:$] := 7};
  }
endclass

module t;
  initial begin
    Packet p;
    SignedPacket s;
    int low, high, si;
    bit [15:0] values;
    p = new;
    s = new;
    for (int pass = 0; pass < 2; pass++) begin
      p.limit = (pass != 0) ? 11 : 3;
      low = 0;
      high = 0;
      repeat (400) begin
        if (p.randomize() != 1) $stop;
        if (int'(p.offset) <= p.limit) low++;
        else high++;
      end
      // :/ gives a weight to each complete range, independent of its size.
      if (low < 244 || low > 356 || high == 0) $stop;
      p.offset.rand_mode(0);
      for (int k = 0; k < 16; ++k) begin
        p.offset = 4'(k);
        if (p.randomize() != 1 || p.offset != 4'(k)) $stop;
      end
      p.offset.rand_mode(1);
    end
    low = 0;
    high = 0;
    values = 0;
    repeat (600) begin
      if (s.randomize() != 1) $stop;
      si = int'(s.offset);
      values[$unsigned(s.offset)] = 1;
      if (si >= -8 && si <= -2) low++;
      else if (si == 6 || si == 7) high++;
      else $stop;
    end
    // Seven low values have weight 1; the two high values each have weight 7.
    if (low < 140 || low > 260 || high == 0 || values != 16'h7fc0) $stop;
    $write("*-* All Finished *-*\n");
    $finish;
  end
endmodule
