// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain.
// SPDX-FileCopyrightText: 2026 Antmicro Ltd
// SPDX-License-Identifier: CC0-1.0

module t;
  bit direct_after;
  bit nested_after;
  bit virtual_deep_after;
  bit virtual_caller_after;
  bit external_after;
  process direct_process;
  process nested_process;
  process virtual_process;
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
        $display("direct_after");
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

    if (direct_after) $fatal(1, "direct_after failed");

    if (nested_after) $fatal(1, "nested_after failed");

    if (virtual_deep_after) $fatal(1, "virtual_deep_after failed");

    if (virtual_caller_after) $fatal(1, "virtual_caller_after failed");

    if (!external_after) $fatal(1, "external_after failed");

    if (direct_process.status() != process::KILLED) $fatal(1, "direct process not killed");

    if (nested_process.status() != process::KILLED) $fatal(1, "nested process not killed");

    if (virtual_process.status() != process::KILLED) $fatal(1, "virtual process not killed");

    if (victim.status() != process::KILLED) $fatal(1, "external victim not killed");
    $write("*-* All Finished *-*\n");
    $finish;
  end
endmodule
