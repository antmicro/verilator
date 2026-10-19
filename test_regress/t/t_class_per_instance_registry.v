// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain.
// SPDX-FileCopyrightText: 2026 Antmicro
// SPDX-License-Identifier: CC0-1.0

package reg_pkg;
  virtual class base_obj;
    pure virtual function int run();
  endclass
  base_obj all[$];
  class registry #(
      type T = int,
      string NAME = ""
  );
    static function void make();
      T t = new;
      all.push_back(t);
    endfunction
  endclass
endpackage

module agent;
  int cfg;
  class drv extends reg_pkg::base_obj;
    virtual function int run();
      return cfg;
    endfunction
  endclass
  class outer;
    class mon extends reg_pkg::base_obj;
      virtual function int run();
        return cfg * 2;
      endfunction
    endclass
  endclass
  // Constructed by the package class, once in each instance of agent
  typedef reg_pkg::registry#(drv, "drv") drv_reg;
  typedef reg_pkg::registry#(outer::mon, "mon") mon_reg;
  initial begin
    #1;
    drv_reg::make();
    mon_reg::make();
  end
endmodule

module t;
  int top_cfg = 7;
  class my_test extends reg_pkg::base_obj;
    virtual function int run();
      return top_cfg;
    endfunction
  endclass
  // One instance: needs no instance reference
  typedef reg_pkg::registry#(my_test, "my_test") test_reg;
  agent a1 (), a2 ();
  initial begin
    a1.cfg = 11;
    a2.cfg = 22;
    test_reg::make();
    #2;
    // my_test, then drv and mon of a1 and a2
    if (reg_pkg::all[0].run() != 7) $stop;
    if (reg_pkg::all[1].run() != 11) $stop;
    if (reg_pkg::all[2].run() != 22) $stop;
    if (reg_pkg::all[3].run() != 22) $stop;
    if (reg_pkg::all[4].run() != 44) $stop;
    $write("*-* All Finished *-*\n");
    $finish;
  end
endmodule
