// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain.
// SPDX-FileCopyrightText: 2026 Antmicro Ltd
// SPDX-License-Identifier: CC0-1.0

// verilog_format: off
`define stop $stop
`define checkd(gotv, expv) \
  do if ((gotv) !== (expv)) begin $write("%%Error: %s:%0d: got=%0d exp=%0d\n", `__FILE__, `__LINE__, (gotv), (expv)); `stop; end while (0)
// verilog_format: on

module t;
  bit direct_after;
  bit nested_after;
  bit virtual_deep_after;
  bit virtual_caller_after;
  bit external_after;
  bit inactive_after;
  process direct_process;
  process nested_process;
  process virtual_process;
  process inactive_process;
  process victim;

  class killer;
    function automatic void kill_deep(process p);
      if (1) begin
        repeat (1) begin
          p.kill();
        end
      end
    endfunction

    function automatic void call_kill_deep(process p);
      kill_deep(p);
    endfunction
  endclass

  class base_killer;
    virtual function automatic void kill_virtual(process p);
    endfunction
  endclass

  class derived_killer extends base_killer;
    function automatic void kill_virtual(process p);
      p.kill();
      virtual_deep_after = 1'b1;
    endfunction
  endclass

  initial begin
    automatic killer k = new();
    automatic derived_killer derived = new();
    automatic base_killer polymorphic = derived;
    fork
      begin
        automatic process p = process::self();
        direct_process = p;
        #1;
        p.kill();
        direct_after = 1'b1;
      end
      begin
        automatic process p = process::self();
        nested_process = p;
        #2;
        k.call_kill_deep(p);
        nested_after = 1'b1;
      end
      begin
        automatic process p = process::self();
        virtual_process = p;
        #3;
        polymorphic.kill_virtual(p);
        virtual_caller_after = 1'b1;
      end
      begin
        automatic process p = process::self();
        inactive_process = p;
        #0;
        p.kill();
        inactive_after = 1'b1;
      end
    join_none

    fork
      begin
        victim = process::self();
        #20;
      end
    join_none
    wait (victim != null);
    #3;
    victim.kill();
    external_after = 1'b1;
    #10;

    `checkd(direct_after, 1'b0);
    `checkd(nested_after, 1'b0);
    `checkd(virtual_deep_after, 1'b0);
    `checkd(virtual_caller_after, 1'b0);
    `checkd(inactive_after, 1'b0);
    `checkd(external_after, 1'b1);
    `checkd(direct_process.status(), process::KILLED);
    `checkd(nested_process.status(), process::KILLED);
    `checkd(virtual_process.status(), process::KILLED);
    `checkd(inactive_process.status(), process::KILLED);
    `checkd(victim.status(), process::KILLED);
    $write("*-* All Finished *-*\n");
    $finish;
  end
endmodule
