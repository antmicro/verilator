// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain.
// SPDX-FileCopyrightText: 2026 OpenAI
// SPDX-License-Identifier: CC0-1.0

class Packet;
  rand bit [7:0] frozen[$];
  rand bit [7:0] lookup[int];
  rand bit [7:0] grow[$];
  int wanted, pre_count, post_count;
  bit freeze, impossible;

  constraint legal {
    frozen.size() == wanted;
    grow.size() == 4;
    lookup[0] == 0;
    if (pre_count >= 2) lookup[900] == 43;
    foreach (grow[i]) {
      grow[i] == 8'(40+i);
      if (impossible && i >= 2) grow[i] == 0;
    }
  }

  function void pre_randomize();
    ++pre_count;
    frozen.rand_mode(!freeze);
    lookup.rand_mode(0);
    if (pre_count == 2) begin
      lookup.delete(104);
      lookup[900] = 43;
    end
  endfunction

  function void post_randomize();
    ++post_count;
  endfunction

  function int attempt(bit inline_call);
    if (inline_call) return this.randomize() with { grow[0] == 40; };
    return this.randomize();
  endfunction

  function void check(bit inline_call, ref bit [7:0] held, ref bit [7:0] entry);
    int ok;
    freeze = 1;
    wanted = 4;
    ok = attempt(inline_call);
    $display("mismatch mode=%0d ok=%0d size=%0d", inline_call, ok, frozen.size());
    if (ok != 0 || pre_count != 1 || post_count != 0 || frozen.size() != 2 ||
        frozen[0] != 17 || frozen[1] != 18 || grow.size() != 1 || grow[0] != 29) $stop;
    wanted = 2;
    impossible = 1;
    ok = attempt(inline_call);
    if (ok != 0 || pre_count != 2 || post_count != 0 || frozen.size() != 2 ||
        grow.size() != 1 || grow[0] != 29) $stop;
    held = 81;
    entry = 82;
    if (frozen[1] != 81 || lookup[27] != 82) $stop;
    impossible = 0;
    ok = attempt(inline_call);
    if (ok != 1 || pre_count != 3 || post_count != 1 || frozen.size() != 2 ||
        frozen[0] != 17 || frozen[1] != 81 || grow.size() != 4 || grow[3] != 43) $stop;
    if (lookup.exists(104) || lookup[900] != 43) $stop;
    foreach (lookup[i]) if (i != 27 && i != 900 && lookup[i] != 8'(i)) $stop;
    freeze = 0;
    wanted = 4;
    ok = attempt(inline_call);
    if (ok != 1 || pre_count != 4 || post_count != 2 || frozen.size() != 4) $stop;
    if (lookup.num() != 128 || lookup[27] != 82) $stop;
  endfunction
endclass

class InlinePacket;
  rand bit [7:0] data[];
  function int attempt(); return this.randomize() with { data.size() == 4; }; endfunction
endclass

module t;
  initial begin
    InlinePacket ip;
    ip = new;
    ip.data = '{17, 18};
    ip.data.rand_mode(0);
    if (ip.attempt() != 0 || ip.data.size() != 2 || ip.data[1] != 18) $stop;
    ip.data.rand_mode(1);
    if (ip.attempt() != 1 || ip.data.size() != 4) $stop;
    for (int i = 0; i < 2; ++i) begin
      Packet p;
      p = new;
      p.frozen = '{17, 18};
      p.grow = '{29};
      for (int j = 0; j < 128; ++j) p.lookup[j] = 8'(j);
      p.check(1'(i), p.frozen[1], p.lookup[27]);
    end
    $write("*-* All Finished *-*\n");
    $finish;
  end
endmodule
