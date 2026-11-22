// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain.
// SPDX-FileCopyrightText: 2020 Wilson Snyder
// SPDX-License-Identifier: CC0-1.0

class Base1;
  task meth1;
  endtask
endclass

class Cls2 extends Base1;
  task meth2;
  endtask
endclass

module t;
  initial begin
    Cls2 c;
    bit [2:0] concatenated;
    bit [1:0] repeated;
    c.meth3();
    concatenated = {c.missing.read(), c.missing.read(), c.missing.read()};
    repeated = {2{c.missing.read()}};
  end
endmodule
