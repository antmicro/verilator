// DESCRIPTION: Verilator: wide queue and dynamic array elements passed by reference
//
// This file ONLY is placed under the Creative Commons Public Domain.
// SPDX-FileCopyrightText: 2026 The Verilator Authors
// SPDX-License-Identifier: CC0-1.0

package p;
  typedef bit [130:0] word_t;
  class holder;
    word_t words[$];
    word_t dyn[];
    bit [31:0] narrow[$];
    int index_calls;

    function int next_index();
      return ++index_calls;
    endfunction

    function int flip(ref word_t word_arg, input int bitno);
      word_arg[bitno] = ~word_arg[bitno];
      return bitno;
    endfunction
  endclass

  function automatic bit aliases(ref word_t a, const ref word_t b, input word_t snapshot, input int bitno);
    a[bitno] = ~a[bitno];
    return b[bitno] != snapshot[bitno];
  endfunction

  function automatic bit flip_narrow(ref bit [31:0] word_arg, input int bitno);
    word_arg[bitno] = ~word_arg[bitno];
    return word_arg[bitno];
  endfunction
endpackage

module t;
  import p::*;
  initial begin
    automatic holder h = new;
    automatic word_t guard_word = '1;
    automatic word_t old;
    h.words = {guard_word, 131'd0, guard_word};
    h.dyn = new[2];
    h.narrow = {32'd0, 32'd0};

    // Index once; make sure the returned value and the change in the queue agree.
    if (h.flip(h.words[h.next_index()], 113) != 113) $stop;
    if (h.index_calls != 1 || h.words[1] != (131'd1 << 113)) $stop;
    if (h.words[0] != guard_word || h.words[2] != guard_word) $stop;

    // A by-value read is a snapshot; a const ref to the same element is a live alias.
    old = h.words[$-1];
    if (!aliases(h.words[$-1], h.words[1], h.words[1], 96)) $stop;
    if (old != (131'd1 << 113) || h.words[1] != (old | (131'd1 << 96))) $stop;

    if (!aliases(h.words[1], h.words[$-1], h.words[$-1], 96)) $stop;
    if (h.words[1] != old || h.words[0] != guard_word || h.words[2] != guard_word) $stop;

    if (h.flip(h.dyn[1], 130) != 130) $stop;
    if (h.dyn[1] != (131'd1 << 130) || h.dyn[0] != 0) $stop;
    if (!flip_narrow(h.narrow[$], 26)) $stop;
    if (h.narrow[1] != (32'd1 << 26) || h.narrow[0] != 0) $stop;

    $write("*-* All Finished *-*\n");
    $finish;
  end
endmodule
