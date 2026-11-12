// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain.
// SPDX-FileCopyrightText: 2026 The Verilator Contributors
// SPDX-License-Identifier: CC0-1.0

module t;
  int lateValue, lateFuncValue;

  task automatic readValue(input int delayTicks, input int data, output int value);
    repeat (delayTicks) #1;
    value = data;
  endtask

  task automatic raceRead(input int delayTicks, input int data, output int value,
                          inout int attempts);
    fork
      begin
        fork
          begin
            int readback;
            readValue(delayTicks, data, readback);
            value = readback;
            attempts++;
          end
          begin
            #4;
            value = -1;
          end
        join_any
        disable fork;
      end
    join
  endtask

  function automatic int backgroundFunc(output int value);
    backgroundFunc = 1;
    value = 2;
    fork
      begin
        #2;
        backgroundFunc = 6;
        value = 7;
        lateFuncValue = backgroundFunc + value;
      end
    join_none
  endfunction

  task automatic background(output int value, inout int tally);
    value = 3;
    fork
      begin
        #2;
        value = 8;
        tally++;
        lateValue = value + tally;
      end
    join_none
  endtask

  initial begin
    int fast, slow, pendingValue, funcResult, funcValue;
    automatic int nFast = 10, nSlow = 20, tally = 4;
    fork
      raceRead(1, 42, fast, nFast);
      raceRead(8, 99, slow, nSlow);
    join
    if ($time != 4 || fast != 42 || nFast != 11 || slow != -1 || nSlow != 20) $stop;
    background(pendingValue, tally);
    funcResult = backgroundFunc(funcValue);
    if (pendingValue != 3 || tally != 4 || funcResult != 1 || funcValue != 2) $stop;
    #5;
    if (lateValue != 13 || pendingValue != 3 || tally != 4 || slow != -1) $stop;
    if (lateFuncValue != 13 || funcResult != 1 || funcValue != 2) $stop;
    $write("*-* All Finished *-*\n");
    $finish;
  end
endmodule
