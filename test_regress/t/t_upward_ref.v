// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain.
// SPDX-FileCopyrightText: 2026 Antmicro
// SPDX-License-Identifier: CC0-1.0

// verilog_format: off
`define stop $stop
`define checkd(gotv,expv) do if ((gotv) !== (expv)) begin $write("%%Error: %s:%0d:  got=%0d exp=%0d\n", `__FILE__,`__LINE__, (gotv), (expv)); `stop; end while(0)
// verilog_format: on

interface caller_if;
  task automatic invoke();
    update();
  endtask
endinterface

module mul;
  int mul_count = 1;
  caller_if caller();

  task automatic update();
    mul_count = mul_count * 2;
  endtask
endmodule

module add;
  int add_count = 0;
  caller_if caller();

  function void update;
    add_count++;
  endfunction
endmodule

interface bit_getter;
  function automatic int get_bits();
    return $bits(get_value());
  endfunction
endinterface

module shared_branch;
  bit_getter getter();
endmodule

module wrapper_a;
  shared_branch branch();
  function automatic logic [15:0] get_value();
    return 16'h1234;
  endfunction
endmodule

module wrapper_b;
  shared_branch branch();
  function automatic logic [31:0] get_value();
    return 32'h1234_1234;
  endfunction
endmodule

module scoped_parent;
  begin : narrow
    shared_branch branch();
    function automatic logic [6:0] get_value();
      return 7'h12;
    endfunction
  end

  begin : wide
    shared_branch branch();
    function automatic logic [32:0] get_value();
      return 33'h1234;
    endfunction
  end

  begin : wider
    shared_branch branch();
    function automatic logic [64:0] get_value();
      return 65'h1234;
    endfunction
  end
endmodule

function int f(int y);
  return y + 1;
endfunction

package p;
  function void g();
    $display("p::g");
  endfunction
endpackage

interface upward_if;
  function automatic int apply(int value);
    return transform(value);  // Resolved in the enclosing parent.
  endfunction
endinterface

module iface_port_consumer(upward_if bus);
  function automatic int evaluate(int value);
    return bus.apply(value);
  endfunction
endmodule

module iface_port_forwarding(upward_if bus);
  iface_port_consumer leaf(.bus(bus));

  function automatic int evaluate(int value);
    return leaf.evaluate(value);
  endfunction
endmodule

module iface_port_parent_a;
  upward_if bus();
  iface_port_forwarding forwarder(.bus(bus));

  function automatic int transform(int value);
    return value + 19;
  endfunction
endmodule

package external;
  function automatic int transform(int value);
    return value + 29;
  endfunction
endpackage

module iface_port_parent_b;
  import external::*;
  upward_if bus();
  iface_port_forwarding forwarder(.bus(bus));
endmodule

module t;
  // Setup for IEEE 1800-2023 23.8.1 Example 2, with 'f' renamed to 'g' to avoid collisions.
  import p::*;
  if (1) begin : b
    bit g_called;
    function void call_g();
      g();
    endfunction
    function void g();
      g_called = 1;
      $display("top.b.g");
    endfunction
  end

  mul mul();
  add add();

  wrapper_a wrapper_a();
  wrapper_b wrapper_b();
  scoped_parent scoped();
  iface_port_parent_a iface_port_a();
  iface_port_parent_b iface_port_b();

  initial begin
    int x;
    static int value = 20;

    // IEEE 1800-2023 23.8.1 Example 2: The unqualified call in b.call_g resolves to b.g.
    b.call_g();
    `checkd(b.g_called, 1);

    // IEEE 1800-2023 23.8.1 Example 1: The unqualified call resolves to compilation-unit f.
    x = f(1);
    `checkd(x, 2);

    // Upward referencing should connect references
    // to tasks/functions in relevant instance hierarchy.
    mul.caller.invoke();
    `checkd(mul.mul_count, 2);
    add.caller.invoke();
    `checkd(add.add_count, 1);

    // Upward referenced function should have correct return type.
    x = wrapper_a.branch.getter.get_bits();
    `checkd(x, 16);
    x = wrapper_b.branch.getter.get_bits();
    `checkd(x, 32);
    x = scoped.narrow.branch.getter.get_bits();
    `checkd(x, 7);
    x = scoped.wide.branch.getter.get_bits();
    `checkd(x, 33);
    x = scoped.wider.branch.getter.get_bits();
    `checkd(x, 65);

    // Check interface variants passed through nested module ports.
    x = iface_port_a.forwarder.evaluate(value);
    `checkd(x, value + 19);
    x = iface_port_b.forwarder.evaluate(value);
    `checkd(x, value + 29);

    $write("*-* All Finished *-*\n");
    $finish;
  end
endmodule
