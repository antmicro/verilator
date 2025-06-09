// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed into the Public Domain, for any use,
// without warranty, 2015 by Mike Thyer.
// SPDX-License-Identifier: CC0-1.0

`define stop $stop
`define checkh(gotv,expv) do if ((gotv) !== (expv)) begin $write("%%Error: %s:%0d:  got='h%x exp='h%x\n", `__FILE__,`__LINE__, (gotv), (expv)); `stop; end while(0)

primitive d_edge_ff (q, clock, data);
 output q; reg q;
 input clock, data;
 initial q = 1'b1;
 table
     // clock data q q+
      // obtain output on rising edge of clock
     F    0 : ? : 0 ;
     (10) 1 : ? : 1 ;
     R    0 : ? : 1 ;
     (0?) 1 : ? : 0 ;
     //? (??) : ? : - ;
 endtable
endprimitive

module t (/*AUTOARG*/
   // Inputs
   clk
   );
   input clk;
   //logic clk = 0; initial forever clk = #1 ~clk;
   reg d, q;
   d_edge_ff g (q, clk, d);

   int cycle=0;
   initial d = 0;
   always @(posedge clk or negedge clk) begin
        $display("d=%d q=%d clk=%d cycle=%0d", d, q, clk, cycle);
     cycle <= cycle+1;
     if (cycle==0) begin
        d = 1;
        d = 0;
     end
     else if (cycle==1) begin
        `checkh(q, 1);
     end
     else if (cycle==2) begin
        `checkh(q, 0);
     end
     else if (cycle==3) begin
        `checkh(q, 1);
     end
     else if (cycle==4) begin
        d = 1;
        `checkh(q, 0);
     end
     else if (cycle==5) begin
        $display("d=%d q=%d clk=%d cycle=%0d", d, q, clk, cycle);
        `checkh(q, 0);
     end
     else if (cycle==6) begin
        `checkh(q, 1);
     end
     else if (cycle==7) begin
        `checkh(q, 0);
     end
     else if (cycle >= 8) begin
        `checkh(q, 1);
        $write("*-* All Finished *-*\n");
        $finish;
     end
   end
endmodule

