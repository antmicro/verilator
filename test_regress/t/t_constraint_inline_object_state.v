// DESCRIPTION: Verilator: Read nested caller state in inline randomization
// SPDX-FileCopyrightText: 2026 The Verilator Authors
// SPDX-License-Identifier: CC0-1.0

class element;
  rand bit [77:0] value;
  rand bit [3:0] tag;
endclass

class generator;
  rand element pattern;
  rand element dynamic_pattern[];
  rand element fixed_pattern[3];
  rand element assoc_pattern[int];
  function new;
    pattern = new;
  endfunction
  function void draw(element result);
    if (result.randomize() with {
      result.value == pattern.value;
      result.tag == pattern.tag;
    } != 1) $stop;
  endfunction

  // Indexed fields on the caller are state even when the arrays and fields are rand.
  // Only the selected destination may change.
  function void draw_indexed();
    element dst[3];
    dynamic_pattern = new[3];
    foreach (dst[i]) begin
      dst[i] = new;
      dynamic_pattern[i] = new;
      fixed_pattern[i] = dynamic_pattern[i];
      assoc_pattern[i] = dynamic_pattern[i];
      dynamic_pattern[i].tag = 4'(3 * i + 1);
      dynamic_pattern[i].value = 78'(i + 1) << 65;
    end
    foreach (dst[i]) begin
      if (dst[i].randomize() with {
            dst[i].tag == dynamic_pattern[i].tag;
            dst[i].value == fixed_pattern[i].value + 78'(assoc_pattern[i].tag);
          } != 1)
        $stop;
      if (dst[i].tag != 4'(3 * i + 1) || dst[i].value != (78'(i + 1) << 65) + 78'(3 * i + 1)) $stop;
      if (dynamic_pattern[i].tag != 4'(3 * i + 1) || dynamic_pattern[i].value != (78'(i + 1) << 65))
        $stop;
    end
  endfunction
endclass

module t;
  initial begin
    automatic generator g = new;
    automatic element x = new;
    g.pattern.value = 78'h3123_4567_89ab_cdef_1234;
    g.pattern.tag = 9;
    g.draw(x);
    if (x.value != g.pattern.value || x.tag != 9) $stop;
    g.pattern.value = ~g.pattern.value;
    g.pattern.tag = 3;
    g.draw(x);
    g.draw_indexed();
    if (x.value != g.pattern.value || x.tag != 3) $stop;
    $display("*-* All Finished *-*");
    $finish;
  end
endmodule
