// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain.
// SPDX-FileCopyrightText: 2026 Antmicro
// SPDX-License-Identifier: CC0-1.0

// verilog_format: off
`define stop $stop
`define checkd(gotv,expv) do if ((gotv) !== (expv)) begin $write("%%Error: %s:%0d:  got=%0d exp=%0d\n", `__FILE__,`__LINE__, (gotv), (expv)); `stop; end while(0);
// verilog_format: on

class FixedSizePacket;
  rand int unsigned length;
  rand bit [7:0] data[];

  constraint contents {data.size() == 3;}
endclass

class LengthSizePacket;
  rand int unsigned length;
  rand bit [7:0] data[];

  constraint contents {
    data.size() == length;
  }
endclass

module t;
  initial begin
    automatic FixedSizePacket fixed_size_packet = new;
    automatic LengthSizePacket length_size_packet = new;
    automatic int randomize_result;

    randomize_result = fixed_size_packet.randomize() with {length == 2;};
    `checkd(randomize_result, 1)
    `checkd(fixed_size_packet.length, 2)
    `checkd(fixed_size_packet.data.size(), 3)

    randomize_result = length_size_packet.randomize() with {length == 1;};
    `checkd(randomize_result, 1)
    `checkd(length_size_packet.length, 1)
    `checkd(length_size_packet.data.size(), 1)

    randomize_result = length_size_packet.randomize() with {length % 3 == 0; data.size() == 9; };
    `checkd(randomize_result, 1)
    `checkd(length_size_packet.length, 9)
    `checkd(length_size_packet.data.size(), 9)

    $write("*-* All Finished *-*\n");
    $finish;
  end
endmodule
