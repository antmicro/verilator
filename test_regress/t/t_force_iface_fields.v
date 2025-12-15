// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain, for
// any use, without warranty, 2025 by Antmicro.
// SPDX-License-Identifier: CC0-1.0

interface iface ();
    bit flag;
endinterface
module t;
    iface if_inst1 ();
    iface if_inst2 ();
    initial begin
        force if_inst1.flag = 1'b0;
    end
    initial begin
        force if_inst2.flag = 1'b0;
    end
    initial $finish;
endmodule
