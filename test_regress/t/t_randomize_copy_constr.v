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

class Instr;
  rand bit [31:0] imm;
endclass

class CompressedInstr extends Instr;
  constraint compressed_imm_c {imm != 0;}
endclass

module t;
  initial begin
    Instr copied;
    Instr instr_for_copy;
    CompressedInstr compressed_template;
    int ok;

    compressed_template = new;
    instr_for_copy = compressed_template;
    copied = new instr_for_copy;
    `check_rand(copied, copied.imm, copied.imm != 0 && compressed_template.imm == 0);

    $write("*-* All Finished *-*\n");
    $finish;
  end
endmodule
