// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain, for
// any use, without warranty, 2025 by Antmicro.
// SPDX-License-Identifier: CC0-1.0

typedef class Bar;

module t;
    initial begin
        Bar::Qux::boo();
        $finish;
    end
endmodule

class Foo #(type T);
    static function void boo();
        $display("boo");
        $finish;
    endfunction
endclass

class Bar;
    typedef Foo#(Bar) Qux;
endclass
