// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain
// SPDX-FileCopyrightText: 2026 Wilson Snyder
// SPDX-License-Identifier: CC0-1.0

package config_pkg;
  bit [3:0] allowed[];
  bit [3:0] tag;

  function automatic bit [3:0] get_tag();
    return tag;
  endfunction
endpackage

package other_pkg;
  bit [3:0] tag;
endpackage

package classes_pkg;
  import config_pkg::tag;

  class Item;
    rand bit [3:0] data, code, sum, from_func;
    bit [3:0] allowed = 4'hf;

    function bit [3:0] get_tag();
      return 4'hf;
    endfunction
  endclass

  class Caller;
    function int choose(Item item);
      return item.randomize() with {
        data inside {config_pkg::allowed};
        code == tag;
        sum == tag + other_pkg::tag;
        from_func == config_pkg::get_tag();
      };
    endfunction
  endclass
endpackage

module t;
  initial begin
    automatic classes_pkg::Item item = new;
    automatic classes_pkg::Caller caller = new;
    config_pkg::allowed = '{4'd1};
    config_pkg::tag = 4'd2;
    other_pkg::tag = 4'd3;
    if (caller.choose(
            item
        ) != 1 || item.data != 1 || item.code != 2 || item.sum != 5 || item.from_func != 2)
      $stop;
    config_pkg::allowed = '{4'd9};
    config_pkg::tag = 4'd7;
    other_pkg::tag = 4'd5;
    if (caller.choose(
            item
        ) != 1 || item.data != 9 || item.code != 7 || item.sum != 12 || item.from_func != 7)
      $stop;
    $display("*-* All Finished *-*");
    $finish;
  end
endmodule
