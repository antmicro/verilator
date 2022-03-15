// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain, for
// any use, without warranty, 2022 by Antmicro Ltd.
// SPDX-License-Identifier: CC0-1.0

module t;
    // As Verilator doesn't support recursive calls, let's use macros to
    // generate tasks for a deep call stack
    `define DEEP_STACK_DELAY_END(i) \
    task delay``i; \
        $write("[%0t] at depth %0d\n", $time, i); \
    endtask

    `define DEEP_STACK_DELAY(i, j) \
    task delay``i; \
        $write("[%0t] entering depth %0d\n", $time, i); \
        #1 delay``j; \
        #1 $write("[%0t] leaving depth %0d\n", $time, i); \
    endtask

    `DEEP_STACK_DELAY_END(10);
    `DEEP_STACK_DELAY(9, 10);
    `DEEP_STACK_DELAY(8, 9);
    `DEEP_STACK_DELAY(7, 8);
    `DEEP_STACK_DELAY(6, 7);
    `DEEP_STACK_DELAY(5, 6);
    `DEEP_STACK_DELAY(4, 5);
    `DEEP_STACK_DELAY(3, 4);
    `DEEP_STACK_DELAY(2, 3);
    `DEEP_STACK_DELAY(1, 2);

    initial begin
        delay1;
        $write("*-* All Finished *-*\n");
        $finish;
    end
endmodule
