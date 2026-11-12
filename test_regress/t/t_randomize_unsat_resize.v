// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain.
// SPDX-FileCopyrightText: 2026 <TODO>
// SPDX-License-Identifier: CC0-1.0

class Base;
  rand bit [7:0] other;
  rand bit [72:0] wide;
endclass

class Packet extends Base;
  rand bit [7:0] scalar;
  rand bit [7:0] queue[$];
  rand bit [7:0] shrink_q[$];
  rand bit [7:0] da[];
  rand bit [72:0] fixed_a[2];
  rand bit [7:0] aa[int];
  bit impossible, mutate;
  int pres, posts;
  constraint cfg {
    scalar == 71;
    wide == 73'h193_4abc1234_576abcde;
    queue.size() == 4;
    shrink_q.size() == 1;
    foreach (shrink_q[i]) shrink_q[i] == 37;
    da.size() == 3;
    foreach (da[i]) da[i] == 38;
    foreach (aa[i]) aa[i] == 41;
    foreach (fixed_a[i]) fixed_a[i] == 73'h123_456789ab_cdef0123;
    foreach (queue[i]) {
      queue[i] == 8'(33+i);
      if (impossible && i >= 2) queue[i] == 17;
    }
  }
  function void pre_randomize();
    ++pres;
    if (mutate) queue[1] = 83;
  endfunction
  function void post_randomize(); ++posts; endfunction
  function int attempt(bit use_inline);
    if (use_inline) return this.randomize() with { scalar == 71; };
    return this.randomize();
  endfunction
  function void check_failed(bit use_inline, ref bit [7:0] held, ref bit [7:0] tail, ref bit [7:0] assoc);
    int ok;
    ok = attempt(use_inline);
    if (ok != 0 || scalar != 7 || other != 11 || queue.size() != 2 || queue[0] != 17 ||
        queue[1] != 83 || pres != 1 || posts != 0) $stop;
    if (shrink_q.size() != 3 || shrink_q[2] != 57 || da.size() != 1 || da[0] != 67) $stop;
    if (wide != 73'h1aa_12345678_23456789) $stop;
    if (fixed_a[1] != 73'h1ab_cdef0123_456789ab || aa.num() != 1 || aa[7] != 59) $stop;
    tail = 69;
    assoc = 70;
    if (shrink_q[2] != 69 || aa[7] != 70) $stop;
    held = 32;
    if (queue[0] != 32) $stop;
  endfunction
  function void check_success(bit use_inline, ref bit [7:0] held, ref bit [7:0] assoc);
    int ok;
    impossible = 0;
    mutate = 0;
    ok = attempt(use_inline);
    if (ok != 1 || scalar != 71 || queue.size() != 4 || pres != 2 || posts != 1) $stop;
    if (wide != 73'h193_4abc1234_576abcde) $stop;
    if (shrink_q.size() != 1 || shrink_q[0] != 37 || da.size() != 3 || da[2] != 38) $stop;
    if (aa.num() != 1 || aa[7] != 41 || fixed_a[1] != 73'h123_456789ab_cdef0123) $stop;
    foreach (queue[j]) if (queue[j] != 8'(33+j)) $stop;
    held = 39;
    assoc = 59;
    if (queue[0] != 39 || aa[7] != 59) $stop;
  endfunction
endclass

module t;
  initial begin
    for (int i = 0; i < 2; ++i) begin
      Packet p;
      p = new;
      p.scalar = 7;
      p.other = 11;
      p.wide = 73'h1aa_12345678_23456789;
      p.queue = '{17, 18};
      p.shrink_q = '{55, 56, 57};
      p.da = '{67};
      p.aa[7] = 59;
      p.fixed_a[1] = 73'h1ab_cdef0123_456789ab;
      p.impossible = 1;
      p.mutate = 1;
      p.check_failed(1'(i), p.queue[0], p.shrink_q[2], p.aa[7]);
      p.check_success(1'(i), p.queue[0], p.aa[7]);
    end
    $display("*-* All Finished *-*");
    $finish;
  end
endmodule
