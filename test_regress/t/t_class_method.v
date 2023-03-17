// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain, for
// any use, without warranty, 2020 by Wilson Snyder.
// SPDX-License-Identifier: CC0-1.0

typedef class Cls;

class Cls;
   int imembera;
   function new(int x); imembera = x; endfunction
   function int get_methoda; return imembera; endfunction
   task set_methoda(input int val); imembera = val; endtask
   function void setv_methoda(input int val); imembera = val; endfunction
endclass : Cls

class ClsArr;
   Cls cls[2];
   function new(int a1, int a2);
      cls[0] = new(a1);
      cls[1] = new(a2);
   endfunction
endclass

module t (/*AUTOARG*/);
   initial begin
      Cls c;
      ClsArr c_arr;
      ClsArr c_arr2;
      ClsArr c_arr3;
      ClsArr q[$];
      ClsArr q_a[$]; 
      if (c != null) $stop;
      c = new(10);
      if (c.get_methoda() != 10) $stop;
      c.set_methoda(20);
      if (c.get_methoda() != 20) $stop;
      c.setv_methoda(30);
      if (c.get_methoda() != 30) $stop;
      if (c.get_methoda != 30) $stop;

      c_arr = new(10, 11);
      if (c_arr.cls[0].get_methoda != 10) $stop;

      c_arr2 = new(11, 10);
      c_arr3 = new(5, 11);
      q = '{c_arr, c_arr2, c_arr3};
      q_a = q.find(x) with (x.cls[0].get_methoda() > 5 && x.cls[1].get_methoda() > 10);
      if (q_a[0].cls[0].get_methoda != 10) $stop;

      $write("*-* All Finished *-*\n");
      $finish;
   end
endmodule
