// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain, for
// any use, without warranty, 2022 by Antmicro Ltd.
// SPDX-License-Identifier: CC0-1.0

module t;
   int a = 0;
   int b = 0;
   int c = 0;

   initial begin
     $write("start with a==0, b==0, c==0\n");
     #1 $write("assign 1 to a\n");
     a = 1;
     #1 $write("assign 2 to a\n");  // a==2
     a = 2;
     #1 $write("assign 0 to a\n");
     a = 0;
     #1 $write("assign 2 to a\n");  // 1<a<3
     a = 2;
     #2 $write("assign 2 to b\n");
     b = 2;
     #1 $write("assign 1 to a\n");  // b>a
     a = 1;
     #1 $write("assign 3 to c\n");
     c = 3;
     #1 $write("assign 4 to c\n");  // a+b<c
     c = 4;
     #1 $write("assign 5 to b\n");  // a<b && b>c
     b = 5;
   end

   initial begin
     #0.5 $write("waiting for a==2\n");
     wait(a == 2);
     $write("waited for a==2\n");
     $write("waiting for a<2\n");
     wait(a < 2);
     $write("waited for a<2\n");
     $write("waiting for a==0\n");
     wait(a == 0);
     $write("waited for a==0\n");
     $write("waiting for 1<a<3\n");
     wait(a > 1 && a < 3);
     $write("waited for 1<a<3\n");
     $write("waiting for b>a\n");
     wait(b > a);
     $write("waited for b>a\n");
     $write("waiting for a+b<c\n");
     wait(a + b < c);
     $write("waited for a+b<c\n");
     $write("waiting for a<b && b>c\n");
     wait(a < b && b > c);
     $write("waited for a<b && b>c\n");
     $write("*-* All Finished *-*\n");
     $finish;
   end
endmodule
