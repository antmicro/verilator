// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain, for
// any use, without warranty, 2025 by Antmicro.
// SPDX-License-Identifier: CC0-1.0

module t ();
  initial begin
    fork : fork_block
      begin : blk1
        static int x = 0;
        repeat (10) begin
          x++;
          #2;
        end
      end
      begin : blk2
        static int y = 0;
        #1;
        repeat (10) begin
          y += blk1.x;
          #2;
        end
      end
    join_none

    #22;
    if (fork_block.blk1.x != 10) $stop;
    if (fork_block.blk2.y != 55) $stop;
    $write("*-* All Finished *-*\n");
    $finish;
  end
endmodule
