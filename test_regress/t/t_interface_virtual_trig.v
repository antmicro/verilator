// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain, for
// any use, without warranty, 2023 by Antmicro Ltd.
// SPDX-License-Identifier: CC0-1.0

interface Bus;
   logic [15:0] data;
endinterface

module t(clk);
   input clk;
   integer cyc = 0;
   Bus intf();
   virtual Bus vif = intf;

   initial vif = intf;
   always @(posedge clk) begin
      cyc <= cyc + 1;
      if (cyc == 1) begin
         vif.data = 'hdead;
      end else if (cyc == 2) begin
         vif.data = 'hbeef;
      end
   end

   // Finish on negedge so that $finish is last
   always @(negedge clk)
      if (cyc >= 3) begin
         $write("*-* All Finished *-*\n");
         $finish;
      end

   always_comb $write("[%0t] intf.data==%h\n", $time, intf.data);
endmodule
