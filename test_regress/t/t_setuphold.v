module t (/*AUTOARG*/
   // Inputs
   clk,
   d
   );

   input clk;
   input d;
   wire delayed_CLK;
   wire delayed_D;
   reg notifier;

   specify
      $setuphold (posedge clk, negedge d, 0, 0, notifier,,, delayed_CLK, delayed_D);
   endspecify

   int cyc = 0;

   always @(posedge clk) begin
      cyc <= cyc + 1;
      $display("%d %d", clk, delayed_CLK);
      if (delayed_CLK != clk || delayed_D != d) begin
         $stop;
      end
      if (cyc == 10) begin
         $display("*-* All Finished *-*");
         $finish;
      end
   end
endmodule
