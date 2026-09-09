// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain.
// SPDX-FileCopyrightText: 2026 Antmicro
// SPDX-License-Identifier: CC0-1.0


// verilog_format: off
`define check_rand(cl, field, cond) \
begin \
   automatic longint prev_result; \
   automatic int ok = 0; \
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

class Base;
  typedef struct {
    rand bit [6:0] lo;
    rand bit [8:0] hi;
  } pair_t;

  rand bit [1:0] x;
  rand bit [1:0] y;
  rand pair_t pair;
  rand bit [6:0] fixed_arr[3];
  rand pair_t pair_fixed_arr[3];
  rand int queue[$];
  rand pair_t pair_queue[$];
  rand int assoc[string];
  rand pair_t pair_assoc[string];

  function new();
    queue = '{0, 0, 0};
    pair_queue = '{'{default: 0}, '{default: 0}};
    assoc["a"] = 0;
    assoc["b"] = 0;
    pair_assoc["a"] = '{default: 0};
    pair_assoc["b"] = '{default: 0};
  endfunction

  constraint constr1 {
    x != 0;
    pair.lo != 0;
    pair.hi != 0;
    foreach (fixed_arr[i]) fixed_arr[i] != 0;
    foreach (pair_fixed_arr[i]) {
      pair_fixed_arr[i].lo != 0;
    }
     foreach (queue[i]) {queue[i] != 0; }
    foreach (pair_queue[i]) {
      pair_queue[i].lo != 0;
    }
    foreach (assoc[key]) {assoc[key] inside {[50 : 70]};}
    foreach (pair_assoc[key]) {
      pair_assoc[key].hi != 0;
    }
  }
endclass

class Derv extends Base;
   rand int z;
   constraint constr2 {
      x != 1;
      y % 2 == 1;
      z inside {1, 5};
   }
endclass

module t;
  initial begin
    Base copied;
    Base base_for_copy;
    Derv derv;
    derv = new;
    base_for_copy = derv;
    copied = new base_for_copy;
    `check_rand(copied, copied.x, copied.x > 1 && derv.x == 0);
    `check_rand(copied, copied.y, copied.y % 2 == 1 && derv.y == 0);
    `check_rand(copied, copied.pair.lo, copied.pair.lo != 0 && derv.pair.lo == 0);
    `check_rand(copied, copied.fixed_arr[1],
                copied.fixed_arr[1] != 0 && derv.fixed_arr[1] == 0);
    `check_rand(copied, copied.pair_fixed_arr[2].lo,
                copied.pair_fixed_arr[2].lo != 0 && derv.pair_fixed_arr[2].lo == 0);
    `check_rand(copied, copied.queue[0], copied.queue[0] != 0 && derv.queue[0] == 0);
    `check_rand(copied, copied.pair_queue[1].lo,
                copied.pair_queue[1].lo != 0 && derv.pair_queue[1].lo == 0);
    `check_rand(copied, copied.assoc["a"],
                copied.assoc["a"] >= 50 && copied.assoc["a"] <= 70 && derv.assoc["a"] == 0);
    `check_rand(copied, copied.pair_assoc["a"].hi,
                copied.pair_assoc["a"].hi != 0 && derv.pair_assoc["a"].hi == 0);

     if(derv.z != 0) $stop;
     $cast(derv, copied);
     `check_rand(derv, derv.z, derv.z == 1 || derv.z == 5);

    $write("*-* All Finished *-*\n");
    $finish;
  end
endmodule
