// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain.
// SPDX-FileCopyrightText: 2026 Antmicro
// SPDX-License-Identifier: CC0-1.0

/*verilator lint_off*/
`define stop $stop
`define checkh(gotv,expv) do if ((gotv) !== (expv)) begin $write("%%Error: %s:%0d:  got='h%x exp='h%x\n", `__FILE__,`__LINE__, (gotv), (expv)); `stop; end while(0);
`define check_range(gotv,minv,maxv) do if ((gotv) < (minv) || (gotv) > (maxv)) begin $write("%%Error: %s:%0d:  got=%0d exp=%0d-%0d\n", `__FILE__,`__LINE__, (gotv), (minv), (maxv)); `stop; end while(0);
/*verilator lint_on*/

class SubClass;
  rand int subVal;
  rand int subArr[];
endclass

class IndepClass;
  rand int val;
  rand SubClass sc;

  function new();
    sc = new;
  endfunction
endclass

class BaseClass;
  rand int arr[];
  // Size-constrained array
  constraint bc {
    arr.size < 10;
    arr.size > 5;
  };
endclass

class ExtClass0 extends BaseClass;
  function int randomize_gpr(IndepClass cls);
    return (cls.randomize() with {
      // If with arr.size on array, that's a field of class
      // calling randomize() with
      if (arr.size > 0) {
        val inside {arr};
      }

      // Randomize nested class array size
      sc.subArr.size < 100;
      sc.subArr.size > 0;

      // Randomization of nested class field
      sc.subVal == val;
    });
  endfunction
endclass

class ExtClass1 extends BaseClass;
  // Class that extends BaseClass and uses arr.size variable
  constraint c {
    unique{arr};
    foreach(arr[i]) {
      arr[i] != 0;
    }
  }
endclass

module t;
  ExtClass0 ext0;
  ExtClass1 ext1;
  IndepClass indep;

  initial begin
    indep = new;
    ext0 = new;
    ext1 = new;

    repeat(10) begin
      `checkh(ext0.randomize(), 1);
      `checkh(ext0.randomize_gpr(indep), 1);
      `checkh(indep.val, indep.sc.subVal);

      `checkh(indep.val inside {ext0.arr}, 1);
      `check_range(ext0.arr.size(), 5, 10);
      `check_range(indep.sc.subArr.size(), 0, 100);

      foreach (ext0.arr[i]) begin
        if (ext0.arr[i] == 0) begin
          `stop;
        end
        foreach (ext0.arr[j]) begin
          if (i == j) continue;
          if (ext0.arr[i] == ext0.arr[j]) begin
            `stop;
          end
        end
      end
    end

  $write("*-* All Finished *-*\n");
  $finish();
  end
endmodule
