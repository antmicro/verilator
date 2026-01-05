// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain, for
// any use, without warranty, 2026 by Antmicro.
// SPDX-License-Identifier: CC0-1.0

typedef struct packed {
  logic [31:0] next;
} kv_reg__keyReg__in_t;

typedef struct packed {
  kv_reg__keyReg__in_t [24-1:0][16-1:0] KEY_ENTRY;
} kv_reg__in_t;

module kv;
  kv_reg__in_t kv_reg_hwif_in;
endmodule

module caliptra_top_tb_services;
  kv key_vault1();
  logic [31:0] mlkem_msg_random;
  initial force key_vault1.kv_reg_hwif_in.KEY_ENTRY[0][0].next
      = mlkem_msg_random[31:0];
endmodule
