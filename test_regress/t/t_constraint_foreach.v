// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain.
// SPDX-FileCopyrightText: 2024 Antmicro Ltd
// SPDX-License-Identifier: CC0-1.0

// verilog_format: off
`define check_rand(cl, field, cond) \
begin \
  automatic longint prev_result; \
  automatic int ok; \
  if (!bit'(cl.randomize())) $stop; \
  prev_result = longint'(field); \
  if (!(cond)) $stop; \
  repeat(9) begin \
    longint result; \
    if (!bit'(cl.randomize())) $stop; \
    result = longint'(field); \
    if (!(cond)) $stop; \
    if (result != prev_result) ok = 1; \
    prev_result = result; \
  end \
  if (ok != 1) $stop; \
end
// verilog_format: on

class C;
  rand int x;
  int q[$] = {0, 0, 0, 0, 0};
  constraint fore {
    x < 7;
    foreach(q[i])
      x > i;
    foreach(q[i])  // loop again with the same index name
      x > i;
  };
endclass

class D;
  rand bit posit;
  rand int x;
  int o[$];  // empty
  int p[$] = {1};
  int q[$] = {0, 0, 0, 0, 0};
  constraint fore {
    if (posit == 1) {
      foreach(o[i]) o[i] > 0;
    }
    if (posit == 1) {
      foreach(p[i]) p[i] > 0;
    }
    if (posit == 1) {
      x < 7;
      foreach(q[i])
        x > i;
    } else {
      x > -3;
      foreach(q[i])
        x < i;
    }
  };
endclass

class E;
    typedef struct { rand bit c; }subcfg_t;
    typedef struct { rand bit [7:0]a; rand bit c; subcfg_t subcfg[]; } cfg_t;
    rand cfg_t cfg[];
    rand bit b;

    function new();
        cfg = new [10];
        foreach (cfg[i]) begin
            cfg[i].subcfg = new [1];
        end
    endfunction

    constraint c {
        foreach (cfg[i]) {
            solve b before cfg[i].subcfg[0];
            solve b before cfg[i].a, cfg[i].c;
            cfg[i].subcfg[0].c == b;
            cfg[i].a[7] == b;
            cfg[i].c == b;
        }
    }
endclass

module t;
  initial begin
    automatic C c = new;
    automatic D d = new;
    automatic E e = new;
    `check_rand(c, c.x, 4 < c.x && c.x < 7);
    `check_rand(d, d.posit, (d.posit ? 4 : -3) < d.x && d.x < (d.posit ? 7 : 0));
    foreach (e.cfg[i]) begin
        `check_rand(e, e.cfg[i].a, (e.cfg[i].a[7] == e.b));
        `check_rand(e, e.cfg[i].subcfg[0].c, (e.cfg[i].subcfg[0].c == e.b));
    end
    $write("*-* All Finished *-*\n");
    $finish;
  end
endmodule
