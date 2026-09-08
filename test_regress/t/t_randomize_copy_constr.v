// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain.
// SPDX-FileCopyrightText: 2026 Antmicro
// SPDX-License-Identifier: CC0-1.0

// Test that new <handle> rebinds copied randomizer variable pointers.

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

class Instr;
  typedef struct {
    rand bit [6:0] lo;
    rand bit [8:0] hi;
  } pair_t;

  rand bit [31:0] imm;
  rand pair_t pair;
  rand bit [6:0] fixed_arr[3];
  rand pair_t pair_fixed_arr[2];
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

  constraint instr_c {
    imm != 0;
    pair.lo != 0;
    pair.hi != 0;
    foreach (fixed_arr[i]) fixed_arr[i] != 0;
    foreach (pair_fixed_arr[i]) {
      pair_fixed_arr[i].lo != 0;
      pair_fixed_arr[i].hi != 0;
    }
    foreach (queue[i]) {queue[i] inside {[1 : 127]};}
    foreach (pair_queue[i]) {
      pair_queue[i].lo != 0;
      pair_queue[i].hi != 0;
    }
    foreach (assoc[key]) {assoc[key] inside {[1 : 127]};}
    foreach (pair_assoc[key]) {
      pair_assoc[key].lo != 0;
      pair_assoc[key].hi != 0;
    }
  }
endclass

class CompressedInstr extends Instr;
endclass

module t;
  initial begin
    Instr copied;
    Instr instr_for_copy;
    CompressedInstr compressed_template;
    compressed_template = new;
    instr_for_copy = compressed_template;
    copied = new instr_for_copy;
    `check_rand(copied, copied.imm, copied.imm != 0 && compressed_template.imm == 0);
    `check_rand(copied, copied.pair.lo, compressed_template.pair.lo == 0);
    `check_rand(copied, copied.pair.hi, compressed_template.pair.hi == 0);
    `check_rand(copied, copied.fixed_arr[0],
                copied.fixed_arr[0] != 0 && compressed_template.fixed_arr[0] == 0);
    `check_rand(copied, copied.pair_fixed_arr[0].lo, compressed_template.pair_fixed_arr[0].lo == 0);
    `check_rand(copied, copied.pair_fixed_arr[0].hi, compressed_template.pair_fixed_arr[0].hi == 0);
    `check_rand(copied, copied.queue[0], copied.queue[0] != 0 && compressed_template.queue[0] == 0);
    `check_rand(copied, copied.pair_queue[0].lo, compressed_template.pair_queue[0].lo == 0);
    `check_rand(copied, copied.pair_queue[0].hi, compressed_template.pair_queue[0].hi == 0);
    `check_rand(copied, copied.assoc["a"],
                copied.assoc["a"] != 0 && compressed_template.assoc["a"] == 0);
    `check_rand(copied, copied.pair_assoc["a"].lo, compressed_template.pair_assoc["a"].lo == 0);
    `check_rand(copied, copied.pair_assoc["a"].hi, compressed_template.pair_assoc["a"].hi == 0);

    $write("*-* All Finished *-*\n");
    $finish;
  end
endmodule
