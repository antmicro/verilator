// DESCRIPTION: Verilator: A size constraint creates new array elements that must receive
// random values even when a different array or a nested object is constrained.
// The final solution must retain the length used to allocate the array.
// SPDX-FileCopyrightText: 2026 The Verilator Authors
// SPDX-License-Identifier: CC0-1.0

class Leaf;
  rand byte unsigned tag;
endclass

class Batch;
  rand byte unsigned keys[], samples[];
  constraint sizes {
    keys.size() == 1;
    samples.size() == 3;
  }
endclass

class Owned;
  rand Leaf leaf = new;
  rand byte unsigned keys[], samples[];
  constraint sizes {
    keys.size() == 1;
    samples.size() == 3;
  }
  constraint fields {
    keys[0] == 8'h17;
    leaf.tag == 8'h29;
  }
endclass

class Sized;
  rand Leaf leaf = new;
  rand byte unsigned samples[];
  constraint sizes {
    samples.size() == 3;
    leaf.tag == 8'h49;
  }
endclass

class VariableSize;
  rand Leaf leaf = new;
  rand byte unsigned samples[];
  rand bit [2:0] length;
  rand Leaf extra_leaves[];
  constraint sizes {
    length inside {[3 : 5]};
    samples.size() == int'(length);
    leaf.tag == 8'h59;
  }
endclass

module t;
  initial begin
    bit [2:0] seen_inline, seen_class, seen_size;
    bit [2:0] seen_variable, seen_variable_inline;
    repeat (12) begin
      automatic Batch a = new;
      automatic Owned b = new;
      automatic Sized c = new;
      automatic VariableSize d = new;
      automatic VariableSize e = new;
      if (a.randomize() with {keys[0] == 8'h31;} != 1 || a.keys[0] != 8'h31) $stop;
      if (b.randomize() != 1 || b.keys[0] != 8'h17 || b.leaf.tag != 8'h29) $stop;
      if (c.randomize() != 1 || c.leaf.tag != 8'h49) $stop;
      if (a.samples.size() != 3 || b.samples.size() != 3 || c.samples.size() != 3) $stop;
      if (d.randomize() != 1 || d.leaf.tag != 8'h59 || d.samples.size() != int'(d.length)) $stop;
      if (e.randomize() with {length != 4;} != 1 || e.length == 4) $stop;
      if (e.leaf.tag != 8'h59 || e.samples.size() != int'(e.length)) $stop;
      if (d.length < 3 || d.length > 5 || e.length < 3 || e.length > 5) $stop;
      foreach (a.samples[i]) begin
        seen_inline[i] |= a.samples[i] != 0;
        seen_class[i] |= b.samples[i] != 0;
        seen_size[i] |= c.samples[i] != 0;
        seen_variable[i] |= d.samples[i] != 0;
        seen_variable_inline[i] |= e.samples[i] != 0;
      end
    end
    if (!(&seen_inline) || !(&seen_class) || !(&seen_size)) $stop;
    if (!(&seen_variable) || !(&seen_variable_inline)) $stop;
    $display("*-* All Finished *-*");
    $finish;
  end
endmodule
