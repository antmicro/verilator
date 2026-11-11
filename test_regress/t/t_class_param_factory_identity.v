// DESCRIPTION: Verilator: SystemVerilog regression test
// SPDX-FileCopyrightText: 2026 The Verilator Authors
// SPDX-License-Identifier: CC0-1.0

interface bus #(type T = int);
  T data;
endinterface

class cfg #(type T = int);
  T data;
endclass

class fixed_default #(type C = cfg #(byte));
  C c = new;
endclass

class seqr #(type V = virtual bus, type C = cfg);
  int marker;
endclass

class agent #(type T = int, type V = virtual bus #(T),
              type C = cfg #(int), type S = seqr #(V, C), type M = int);
  S member;
  typedef agent #(T, V, C, S, M) self_t;
  static function self_t create();
    self_t a = new;
    return a;
  endfunction
endclass

module t;
  typedef virtual bus #(byte) vif_t;
  typedef seqr #(vif_t, cfg #(int)) seqr_t;
  typedef agent #(.T(byte), .M(byte)) agent_t;

  initial begin
    automatic agent_t a = agent_t::create();
    automatic seqr_t s = new;
    automatic fixed_default #() fixed_byte = new;
    automatic fixed_default #(cfg #(int)) explicit_int = new;
    if ($bits(fixed_byte.c.data) != 8 || $bits(explicit_int.c.data) != 32) $fatal;
    a.member = s;
    a.member.marker = 12;
    if (s.marker != 12) $fatal;
    $display("*-* All Finished *-*");
    $finish;
  end
endmodule
