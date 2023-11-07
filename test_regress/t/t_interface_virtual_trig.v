// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain, for
// any use, without warranty, 2023 by Antmicro Ltd.
// SPDX-License-Identifier: CC0-1.0

interface Bus1;
   logic [15:0] data;
endinterface

interface Bus2;
   logic [15:0] data;
endinterface

interface Bus3;
   logic [15:0] data;
endinterface

module t(clk);
   input clk;
   integer cyc = 0;
   Bus1 intf1();
   Bus2 intf2();
   Bus3 intf3();
   virtual Bus1 vif1 = intf1;
   virtual Bus2 vif2 = intf2;
   virtual Bus3 vif3 = intf3;

   logic [15:0] data;
   assign vif2.data = data;

   always @(posedge clk) begin
      cyc <= cyc + 1;
      if (cyc == 1) begin
         vif1.data <= 'hdead;
      end else if (cyc == 2) begin
         data <= vif1.data;
      end else if (cyc == 3) begin
         vif1.data <= 'hbeef;
      end else if (cyc == 4) begin
         data <= vif1.data;
      end if (cyc == 5) begin
         intf3.data = 'hfafa;
      end else if (cyc == 6) begin
         intf3.data = 'hbebe;
      end
   end

   // Finish on negedge so that $finish is last
   always @(negedge clk)
      if (cyc >= 7) begin
         $write("*-* All Finished *-*\n");
         $finish;
      end

   always_comb $write("[%0t] intf1.data==%h\n", $time, intf1.data);
   always_comb $write("[%0t] intf2.data==%h\n", $time, intf2.data);
   always_comb $write("[%0t] vif3.data==%h\n", $time, vif3.data);
endmodule
