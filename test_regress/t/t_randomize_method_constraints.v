// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain, for
// any use, without warranty, 2023 by Antmicro Ltd.
// SPDX-License-Identifier: CC0-1.0

class Cls;
   constraint B { w == 5;}
   constraint C { z < 3 * 7; z > 5 + 8; }

   rand logic[63:0] w;
   rand logic[23:0] z;

   function new;
      w = 0;
      z = 0;
   endfunction

endclass

module t (/*AUTOARG*/);
   Cls obj;

   initial begin
      int rand_result;
      longint prev_checksum;
      $display("===================\nSatisfiable constraints:");
      for (int i = 0; i < 25; i++) begin
         obj = new;
         rand_result = obj.randomize();
         $display("obj.w == %0d", obj.w);
         $display("obj.z == %0d", obj.z);
         $display("rand_result == %0d", rand_result);
         $display("-------------------");
         if (obj.w != 5) $stop;
         if (obj.z <= 13 || obj.z >= 21) $stop;
      end
      //$display("===================\nUnsatisfiable constraints for obj.y:");
      //rand_result = obj.randomize() with { 256 < y && y < 256; };
      //$display("obj.y == %0d", obj.y);
      //$display("rand_result == %0d", rand_result);
      //if (rand_result != 0) $stop;
      //rand_result = obj.randomize() with { 16 <= z && z <= 32; };
      $write("*-* All Finished *-*\n");
      $finish;
   end
endmodule
