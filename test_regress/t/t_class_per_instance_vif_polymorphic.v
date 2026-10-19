// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain.
// SPDX-FileCopyrightText: 2026 Antmicro
// SPDX-License-Identifier: CC0-1.0

package drv_pkg;
  virtual class driver;
    pure virtual task drive(int value);
    pure virtual function int sample();
  endclass
endpackage

interface bus;
  int data;
  int writes;
  task write(int value);
    data = value;
    ++writes;
  endtask
  class bus_driver extends drv_pkg::driver;
    virtual task drive(int value);
      // Interface task
      write(value);
    endtask
    virtual function int sample();
      return data;
    endfunction
  endclass
  bus_driver drv = new;
  function drv_pkg::driver get_driver();
    return drv;
  endfunction
endinterface

class agent;
  drv_pkg::driver drv;
  virtual bus vif;
  task run(int value);
    drv.drive(value);
  endtask
endclass

module t;
  bus b1 (), b2 ();
  agent a1 = new, a2 = new;
  initial begin
    // Base class handles to objects of each interface instance
    a1.drv = b1.get_driver();
    a2.drv = b2.get_driver();
    a1.vif = b1;
    a2.vif = b2;
    #1;
    a1.run(10);
    a2.run(20);
    a2.run(21);
    if (b1.data !== 10) $stop;
    if (b1.writes !== 1) $stop;
    if (b2.data !== 21) $stop;
    if (b2.writes !== 2) $stop;
    if (a1.drv.sample() !== 10) $stop;
    if (a2.drv.sample() !== 21) $stop;
    if (a2.vif.drv.sample() !== 21) $stop;
    $write("*-* All Finished *-*\n");
    $finish;
  end
endmodule
