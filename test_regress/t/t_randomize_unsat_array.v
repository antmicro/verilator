// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain.
// SPDX-FileCopyrightText: 2026 <TODO>
// SPDX-License-Identifier: CC0-1.0

class Packet;
  rand bit [7:0] tag;
  rand bit [7:0] q[$];
  rand bit [7:0] da[];
  rand bit [72:0] fixed_a[2];
  rand bit [7:0] aa[int];
  int pre_count, post_count;
  bit edit;

  function int attempt();
    return this.randomize() with { tag == 0; tag == 1; };
  endfunction

  function void pre_randomize();
    ++pre_count;
    if (edit) begin
      q[1] = 83;
      q.push_back(91);
    end
  endfunction
  function void post_randomize();
    ++post_count;
  endfunction
  function void check_failed(ref bit [7:0] held);
    int ok;
    edit = 1;
    ok = attempt();
    if (ok != 0 || pre_count != 1 || post_count != 0 || tag != 4) $stop;
    if (q.size() != 3 || q[0] != 17 || q[1] != 83 || q[2] != 91) $stop;
    if (da.size() != 1 || da[0] != 31) $stop;
    if (fixed_a[1] != 73'h123_456789ab_cdef0123 || aa.num() != 1 || aa[7] != 51) $stop;
    // A failed solve cannot detach a reference to a surviving queue element.
    held = 32;
    if (q[0] != 32) $stop;
  endfunction
endclass

module t;

  initial begin
    Packet p;
    int ok;
    p = new;
    p.tag = 4;
    p.q = '{17, 18};
    p.da = '{31};
    p.fixed_a[1] = 73'h123_456789ab_cdef0123;
    p.aa[7] = 51;
    p.check_failed(p.q[0]);
    p.edit = 0;
    ok = p.randomize() with {
      tag == 7;
      foreach (q[i]) q[i] == 8'(20+i);
      foreach (da[i]) da[i] == 61;
      foreach (fixed_a[i]) fixed_a[i] == 73'h1ab_cdef0123_456789ab;
      foreach (aa[i]) aa[i] == 8'(i+3);
    };
    if (ok != 1 || p.pre_count != 2 || p.post_count != 1 || p.tag != 7) $stop;
    if (p.q.size() != 3 || p.q[0] != 20 || p.q[1] != 21 || p.q[2] != 22) $stop;
    if (p.da.size() != 1 || p.da[0] != 61) $stop;
    foreach (p.fixed_a[i]) if (p.fixed_a[i] != 73'h1ab_cdef0123_456789ab) $stop;
    if (p.aa.num() != 1 || p.aa[7] != 10) $stop;
    $write("*-* All Finished *-*\n");
    $finish;
  end
endmodule
