// DESCRIPTION: Verilator: Verilog Test module for SystemVerilog 'alias'
//
// Simple bi-directional transitive alias test.
//
// This file ONLY is placed under the Creative Commons Public Domain
// SPDX-FileCopyrightText: 2026 Antmicro
// SPDX-License-Identifier: CC0-1.0

package I3CCSR_pkg;
    typedef struct packed{
        logic [7:0] value;
    } I3CCSR__I3C_EC__SecFwRecoveryIf__DEVICE_STATUS_0__DEV_STATUS__out_t;
    typedef struct packed{
        I3CCSR__I3C_EC__SecFwRecoveryIf__DEVICE_STATUS_0__DEV_STATUS__out_t DEV_STATUS; int REC_REASON_CODE;
    } I3CCSR__I3C_EC__SecFwRecoveryIf__DEVICE_STATUS_0__out_t;
    typedef struct packed{
        I3CCSR__I3C_EC__SecFwRecoveryIf__DEVICE_STATUS_0__out_t DEVICE_STATUS_0;
    } I3CCSR__I3C_EC__SecFwRecoveryIf__out_t;
endpackage
module controller (
    input I3CCSR_pkg::I3CCSR__I3C_EC__SecFwRecoveryIf__out_t hwif_rec_i,
);
  localparam int unsigned RecoveryMode = 'h3;
  assign recovery_mode = (hwif_rec_i.DEVICE_STATUS_0.DEV_STATUS.value == RecoveryMode);

  initial begin
    $write("*-* All Finished *-*\n");
    $finish;
  end
endmodule
