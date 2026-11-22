// DESCRIPTION: Verilator: SystemVerilog regression test
// SPDX-FileCopyrightText: 2026 The Verilator Authors
// SPDX-License-Identifier: CC0-1.0

// verilog_format: off
`define stop $stop
`define checkd(gotv,expv) do if ((gotv) !== (expv)) begin $write("%%Error: %s:%0d: got=%0d exp=%0d\n", `__FILE__, `__LINE__, (gotv), (expv)); `stop; end while (0);
// verilog_format: on

class RejectItem;
  rand bit [4:0] value;
  bit reject;
  constraint legal { value == 5; if (reject) value == 6; }
endclass

class RandBase;
  rand bit [6:0] value;
  constraint base_range { value inside {[3:23]}; }
endclass

class RandChild extends RandBase;
  rand bit [14:0] value;
  rand RejectItem item;
  constraint child_range { value inside {[128:255]}; }
  function new();
    item = new;
  endfunction
endclass

class CyclicBase;
  randc bit [2:0] value;
endclass

class CyclicChild extends CyclicBase;
  randc bit [4:0] value;
  rand RejectItem item;
  function new();
    item = new;
  endfunction
endclass

class MixedBase;
  rand bit [6:0] value;
endclass

class MixedChild extends MixedBase;
  randc bit [4:0] value;
  rand RejectItem item;
  function new();
    item = new;
  endfunction
endclass

class ReverseChild extends CyclicBase;
  rand bit [6:0] value;
  rand RejectItem item;
  function new();
    item = new;
  endfunction
endclass

class Third extends RandChild;
  rand bit [9:0] value;
  constraint third_range { int'(value) == 512 + int'(super.value); }
endclass

class PlainBase;
  int value = 41;
endclass

class HidesPlain extends PlainBase;
  rand bit [7:0] value;
  constraint fixed_value { int'(value) == super.value + 1; }
endclass

class PlainChild extends RandBase;
  int value = 61;
endclass

module t;
  initial begin
    automatic Third third = new;
    automatic RandChild middle = third;
    automatic RandBase base_view = third;
    automatic HidesPlain hides_plain = new;
    automatic PlainBase plain_base = hides_plain;
    automatic PlainChild plain_child = new;
    automatic RandBase random_base = plain_child;
    middle.value = 180;
    middle.value.rand_mode(0);
    repeat (12) begin
      `checkd(third.randomize(), 1)
      `checkd(base_view.value >= 3 && base_view.value <= 23, 1)
      `checkd(middle.value, 180)
      `checkd(third.value, 692)
      `checkd(hides_plain.randomize(), 1)
      `checkd(plain_base.value, 41)
      `checkd(hides_plain.value, 42)
      `checkd(plain_child.randomize(), 1)
      `checkd(random_base.value >= 3 && random_base.value <= 23, 1)
      `checkd(plain_child.value, 61)
    end
  end

  initial begin
    automatic RandChild child = new;
    automatic RandBase base_view = child;
    for (int i = 0; i < 64; ++i) begin
      `checkd(base_view.randomize(), 1)
      `checkd(base_view.value >= 3 && base_view.value <= 23, 1)
      `checkd(child.value >= 128 && child.value <= 255, 1)
    end
    base_view.value = 17;
    child.value = 201;
    child.item.value = 29;
    child.item.reject = 1;
    `checkd(base_view.randomize(), 0)
    `checkd(base_view.value, 17)
    `checkd(child.value, 201)
    `checkd(child.item.value, 29)
    child.item.reject = 0;
    `checkd(child.randomize(), 1)
  end

  initial begin
    automatic CyclicChild child = new;
    automatic CyclicBase base_view = child;
    automatic bit [7:0] base_seen = 0;
    automatic bit [31:0] child_seen = 0;
    for (int i = 0; i < 128; ++i) begin
      `checkd(base_view.randomize(), 1)
      `checkd(base_seen[base_view.value], 0)
      `checkd(child_seen[child.value], 0)
      base_seen[base_view.value] = 1;
      child_seen[child.value] = 1;
      if (i % 8 == 7) begin
        `checkd(base_seen, '1)
        base_seen = 0;
      end
      if (i % 32 == 31) begin
        `checkd(child_seen, '1)
        child_seen = 0;
      end
    end
    base_view.value = 3;
    child.value = 21;
    child.item.value = 29;
    child.item.reject = 1;
    `checkd(base_view.randomize(), 0)
    `checkd(base_view.value, 3)
    `checkd(child.value, 21)
    `checkd(child.item.value, 29)
  end

  initial begin
    automatic MixedChild child = new;
    automatic MixedBase base_view = child;
    automatic bit [31:0] child_seen = 0;
    automatic bit [6:0] previous = 0;
    automatic bit changed = 0;
    for (int i = 0; i < 128; ++i) begin
      `checkd(child.randomize(), 1)
      `checkd(child_seen[child.value], 0)
      child_seen[child.value] = 1;
      if (i % 32 == 31) begin
        `checkd(child_seen, '1)
        child_seen = 0;
      end
      if (i != 0 && base_view.value != previous) changed = 1;
      previous = base_view.value;
    end
    `checkd(changed, 1)
    base_view.value = 73;
    child.value = 17;
    child.item.value = 29;
    child.item.reject = 1;
    `checkd(child.randomize(), 0)
    `checkd(base_view.value, 73)
    `checkd(child.value, 17)
    `checkd(child.item.value, 29)
  end

  initial begin
    automatic ReverseChild child = new;
    automatic CyclicBase base_view = child;
    automatic bit [7:0] base_seen = 0;
    automatic bit [6:0] previous = 0;
    automatic bit changed = 0;
    for (int i = 0; i < 128; ++i) begin
      `checkd(child.randomize(), 1)
      `checkd(base_seen[base_view.value], 0)
      base_seen[base_view.value] = 1;
      if (i % 8 == 7) begin
        `checkd(base_seen, '1)
        base_seen = 0;
      end
      if (i != 0 && child.value != previous) changed = 1;
      previous = child.value;
    end
    `checkd(changed, 1)
    base_view.value = 5;
    child.value = 103;
    child.item.value = 29;
    child.item.reject = 1;
    `checkd(child.randomize(), 0)
    `checkd(base_view.value, 5)
    `checkd(child.value, 103)
    `checkd(child.item.value, 29)
  end

  initial begin
    #1;
    $display("*-* All Finished *-*");
    $finish;
  end
endmodule
