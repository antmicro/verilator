// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain, for
// any use, without warranty, 2022 by Antmicro Ltd.
// SPDX-License-Identifier: CC0-1.0

`define checkh(gotv,expv) do if ((gotv) !== (expv)) begin $write("%%Error: %s:%0d:  got='h%x exp='h%x\n", `__FILE__,`__LINE__, (gotv), (expv)); $stop; end while(0)

// function int count_global;
//    int count = 0;
//    count++;
//    return count;
// endfunction

function automatic int count_global_automatic_func;
   static int count = 0;
   count++;
   return count;
endfunction

// package my_pkg;
//    function static int count_in_pkg;
//       int count = 0;
//       count++;
//       return count;
//    endfunction
// endpackage

class MyClass;
   // function static int count_in_static_method;
   //    int count = 0;
   //    count++;
   //    return count;
   // endfunction

   function int count_in_method;
      static int count = 0;
      count++;
      return count;
   endfunction
endclass

module t (/*AUTOARG*/
   );

   function int count_in_module;
      static int count = 0;
      count++;
      return count;
   endfunction

   function int count_in_module_from_10;
      static int count = 9;
      count++;
      return count;
   endfunction

   int           result;
   MyClass cls;
   initial begin
      // result = count_global();
      // `checkh(result, 1);
      // result = count_global();
      // `checkh(result, 2);

      result = count_global_automatic_func();
      `checkh(result, 1);
      result = count_global_automatic_func();
      `checkh(result, 2);

      // result = my_pkg::count_in_pkg();
      // `checkh(result, 1);
      // result = my_pkg::count_in_pkg();
      // `checkh(result, 2);

      // result = MyClass::count_in_static_method();
      // `checkh(result, 1);
      // result = MyClass::count_in_static_method();
      // `checkh(result, 2);

      cls = new;
      result = cls.count_in_method();
      `checkh(result, 1);
      result = cls.count_in_method();
      `checkh(result, 2);

      result = count_in_module();
      `checkh(result, 1);
      result = count_in_module();
      `checkh(result, 2);

      result = count_in_module_from_10();
      `checkh(result, 10);
      result = count_in_module_from_10();
      `checkh(result, 11);

      $write("*-* All Finished *-*\n");
      $finish;
   end
endmodule
