// DESCRIPTION: Verilator: Verilog Test module
//
// SPDX-FileCopyrightText: 2026 The Verilator Authors
// SPDX-License-Identifier: CC0-1.0

// Repeated inline solves on one object exhaust the same feasible randc values,
// including across call sites. Failed solves and check-only calls leave its value alone.
class C;
  randc bit [3:0] tag;
  rand bit [3:0] limit;
  constraint bound {limit >= tag;}
endclass

class InlineOnly;
  randc bit [3:0] tag;
endclass

module t;
  task automatic draw(input C obj, input bit other_site, ref bit [15:0] seen);
    int ok;
    if (other_site) begin
      ok = obj.randomize() with {tag inside {[4 : 7]};};
    end
    else begin
      ok = obj.randomize() with {tag inside {[4 : 7]};};
    end
    if ((ok != 1) || !(obj.tag inside {[4 : 7]}) || obj.limit < obj.tag || seen[obj.tag]) $stop;
    seen[obj.tag] = 1;
  endtask

  initial begin
    C a, b;
    InlineOnly c;
    bit [15:0] seen_a, seen_b, seen_c;
    int ok;
    a = new;
    b = new;
    c = new;
    repeat (2) begin
      seen_a = 0;
      seen_b = 0;
      seen_c = 0;
      for (int i = 0; i < 4; ++i) begin
        draw(a, i[0], seen_a);
        draw(b, !i[0], seen_b);
        ok = c.randomize() with {tag inside {[4 : 7]};};
        if ((ok != 1) || !(c.tag inside {[4 : 7]}) || seen_c[c.tag]) $stop;
        seen_c[c.tag] = 1;
      end
      if (seen_a != 16'h00f0 || seen_b != 16'h00f0 || seen_c != 16'h00f0) $stop;
      begin
        automatic bit [3:0] previous = a.tag;
        ok = a.randomize(null);
        if ((ok != 1) || a.tag != previous) $stop;
        ok = a.randomize() with {
          tag == 0;
          tag == 1;
        };
        if ((ok != 0) || a.tag != previous) $stop;
      end
    end
    $write("*-* All Finished *-*\n");
    $finish;
  end
endmodule
