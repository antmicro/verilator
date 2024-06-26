// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain, for
// any use, without warranty, 2022 by Geza Lore.
// SPDX-License-Identifier: CC0-1.0

`define stop $stop
`define checkh(gotv,expv) do if ((gotv) !== (expv)) begin $write("%%Error: %s:%0d:  got='h%x exp='h%x\n", `__FILE__,`__LINE__, (gotv), (expv)); `stop; end while(0)

module t (
          input wire        clk,
          input wire        rst,
          output reg [31:0] cyc
          );

   always @(posedge clk) begin
      if (rst) begin
         cyc <= 0;
      end else begin
         cyc <= cyc +1;
      end
   end

`ifdef CMT
   wire         net_1 /* verilator forceable */;
   wire [7:0]   net_8 /* verilator forceable */;
`else
   wire         net_1;
   wire [7:0]   net_8;
`endif

   assign net_1 = ~cyc[0];
   assign net_8 = ~cyc[1 +: 8];

   wire         obs_1 = net_1;
   wire [7:0]   obs_8 = net_8;

   always @ (posedge clk) begin
      $display("%d: %x %x", cyc, obs_8, obs_1);

      if (!rst) begin
         case (cyc)
           3: begin
              `checkh (obs_1, 0);
              `checkh (obs_8, ~cyc[1 +: 8]);
           end
           4: begin
              `checkh (obs_1, 0);
              `checkh (obs_8, 8'h5f);
           end
           5: begin
              `checkh (obs_1, 1);
              `checkh (obs_8, 8'h5f);
           end
           6, 7: begin
              `checkh (obs_1, 1);
              `checkh (obs_8, 8'hf5);
           end
           8: begin
              `checkh (obs_1, ~cyc[0]);
              `checkh (obs_8, 8'hf5);
           end
           10, 11: begin
              `checkh (obs_1, 1);
              `checkh (obs_8, 8'h5a);
           end
           12, 13: begin
              `checkh (obs_1, 0);
              `checkh (obs_8, 8'ha5);
           end
           default: begin
              `checkh ({obs_8, obs_1}, ~cyc[0 +: 9]);
           end
         endcase
      end

      if (cyc == 30) begin
         $write("*-* All Finished *-*\n");
         $finish;
      end
   end

endmodule
