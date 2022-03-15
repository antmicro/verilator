// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain, for
// any use, without warranty, 2022 by Antmicro Ltd.
// SPDX-License-Identifier: CC0-1.0

class EventClass;
    event e;

    task sleep; @e; endtask
    task wake; ->e; endtask
endclass

virtual class DelayClass;
    pure virtual task do_delay;
    pure virtual task do_sth_else;
endclass

`define DELAY(dt) \
class Delay``dt extends DelayClass; \
    virtual task do_delay; \
        $write("Starting a #%0d delay\n", dt); \
        #dt \
        $write("Ended a #%0d delay\n", dt); \
    endtask \
    virtual task do_sth_else; \
        $write("Task with no delay (in Delay%0d)\n", dt); \
    endtask \
endclass

`DELAY(10);
`DELAY(20);
`DELAY(40);

class NoDelay extends DelayClass;
    virtual task do_delay;
        $write("Task with no delay\n");
    endtask
    virtual task do_sth_else;
        $write("Task with no delay (in NoDelay)\n");
    endtask
endclass

module t;
    EventClass ec = new;

    initial begin
        @ec.e;
        ec.sleep;
    end

    initial #25 ec.wake;
    initial #50 ->ec.e;

    always @ec.e $write("Event in class triggered at time %0t!\n", $time);

    task printtime;
        $write("I'm at time %0t\n", $time);
    endtask

    initial begin
        DelayClass dc;
        Delay10 d10 = new;
        Delay20 d20 = new;
        Delay40 d40 = new;
        NoDelay dNo = new;
        printtime;
        dc = d10;
        dc.do_delay;
        dc.do_sth_else;
        printtime;
        dc = d20;
        dc.do_delay;
        dc.do_sth_else;
        printtime;
        dc = d40;
        dc.do_delay;
        dc.do_sth_else;
        printtime;
        dc = dNo;
        dc.do_delay;
        dc.do_sth_else;
        printtime;
        $write("*-* All Finished *-*\n");
        $finish;
    end
endmodule
