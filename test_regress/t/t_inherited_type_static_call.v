// DESCRIPTION: Verilator: SystemVerilog regression test
// SPDX-FileCopyrightText: 2026 The Verilator Authors
// SPDX-License-Identifier: CC0-1.0

class driver #(type data_t = int, type cfg_t = int);
  static function int width_sum();
    data_t data_value;
    cfg_t cfg_value;
    return $bits(data_value) + $bits(cfg_value);
  endfunction
endclass

class config_class;
endclass

class base #(type cfg_t = bit, type class_t = config_class);
  typedef bit [2:0] alias_t;
endclass

class agent #(type data_t = bit) extends base #(bit [4:0], config_class);
  function int check();
    return driver #(data_t, cfg_t)::width_sum();
  endfunction
  function int check_alias();
    return driver #(data_t, alias_t)::width_sum();
  endfunction
endclass

module t;
  initial begin
    automatic agent #(bit [7:0]) a = new;
    if (a.check() != 13) $fatal;
    if (a.check_alias() != 11) $fatal;
    $display("*-* All Finished *-*");
    $finish;
  end
endmodule
