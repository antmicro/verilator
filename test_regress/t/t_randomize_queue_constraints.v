// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain, for
// any use, without warranty, 2024 by Antmicro Ltd.
// SPDX-License-Identifier: CC0-1.0

class Foo;
  rand int m_intQueue[$];
  rand int m_idx;

  function new;
    m_intQueue = '{10{0}};
  endfunction

  constraint int_queue_c {
    m_idx inside {[0:9]};
    m_intQueue[m_idx] == m_idx + 1;
    foreach (m_intQueue[i]) {
      m_intQueue[i] inside {[0:127]};
    }
  }
endclass

module t_randomize_queue_constraints;
  initial begin
    Foo foo = new;

    $display("Queue length: %0d", foo.m_intQueue.size());

    if (foo.randomize() != 1) $stop;
    $display("Queue: %p", foo.m_intQueue);
    $write("*-* All Finished *-*\n");
    $finish;
  end
endmodule
