// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain.
// SPDX-FileCopyrightText: 2026 Wilson Snyder
// SPDX-License-Identifier: CC0-1.0

interface channel;
  event wake;
endinterface

class driver;
  virtual channel target;

  function void send();
    ->target.wake;
  endfunction
endclass

module t;
  channel a();
  channel b();
  driver d;
  int count_a, count_b;
  bit via_handle, via_wait;
  virtual channel reader;

  always @(a.wake) count_a++;
  always @(b.wake) count_b++;

  initial begin
    reader = b;
    @(reader.wake);
    via_handle = 1;
  end

  initial begin
    wait (b.wake.triggered);
    via_wait = 1;
  end

  initial begin
    d = new;
    d.target = a;
    #1;
    d.send();
    #0;
    if (count_a != 1 || count_b != 0 || !a.wake.triggered) $stop;
    d.send();
    #0;
    if (count_a != 2 || count_b != 0 || via_handle || via_wait) $stop;
    #1;
    d.target = b;
    d.send();
    #0;
    if (count_a != 2 || count_b != 1 || a.wake.triggered || !b.wake.triggered) $stop;
    if (!via_handle || !via_wait) $stop;
    $write("*-* All Finished *-*\n");
    $finish;
  end
endmodule
