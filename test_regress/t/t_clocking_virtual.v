// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain, for
// any use, without warranty, 2024 by Antmicro Ltd.
// SPDX-License-Identifier: CC0-1.0

interface Iface;
   logic clk = 1'b0;
   logic inp = 1'b0;
   logic io = 1'b0;
   clocking cb @(posedge clk);
       input #7 inp;
       inout io;
   endclocking
   always @(posedge clk) inp <= 1'b1;
   always #5 clk <= ~clk;
endinterface

module main;
   logic clk = 1'b0;
   logic inp = 1'b0;
   always begin
      #6;
      t.mod1.cb.io <= 1'b1;
      if (t.mod0.io != 1'b0) $stop;
      if (t.mod1.cb.io != 1'b0) $stop;
      if (t.mod1.cb.inp != 1'b0) $stop;
      if (t.main1.cbb.inp != 1'b0) $stop;
      if (t.main2.cbb.inp != 1'b0) $stop;
      @(posedge t.mod0.io)
      if ($time != 15) $stop;
      if (t.mod0.io != 1'b1) $stop;
      if (t.mod1.cb.io != 1'b0) $stop;
      #1
      if (t.mod0.cb.io != 1'b1) $stop;
      if (t.mod1.cb.io != 1'b1) $stop;
      if (t.mod1.cb.inp != 1'b1) $stop;
      if (t.main1.cbb.inp != 1'b1) $stop;
      if (t.main2.cbb.inp != 1'b1) $stop;
   end
   clocking cbb @(posedge clk);
       input #7 inp;
   endclocking
   always @(posedge clk) inp <= 1'b1;
   always #5 clk <= ~clk;
endmodule

module t;
   main main1();
   Iface mod0();
   virtual Iface mod1 = mod0;
   main main2();
   initial begin
      #20;
      $write("*-* All Finished *-*\n");
      $finish;
   end
endmodule
