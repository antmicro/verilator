// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain.
// SPDX-FileCopyrightText: 2026 Wilson Snyder
// SPDX-License-Identifier: CC0-1.0

package p;
  class value;
    int n;
    function new(int n);
      this.n = n;
    endfunction
  endclass
  class owner;
    value field;
  endclass
  owner current;
  int calls;
  function automatic owner select_owner();
    calls++;
    return current;
  endfunction
  function automatic int observe(const ref value first, ref value second);
    current = new;
    second = new(27);
    return first.n;
  endfunction
  function automatic void change(ref value v);
    current = new;
    v.n++;
    v = new(12);
  endfunction
  function automatic void forward(ref value v);
    change(v);
  endfunction
endpackage
module t;
  import p::*;
  initial begin
    owner saved;
    value previous;
    current = new;
    saved = current;
    previous = new(4);
    current.field = previous;
    forward(current.field);
    if (previous.n != 5 || current.field != null ||
        saved.field == previous || saved.field.n != 12) $fatal;
    current = saved;
    if (observe(select_owner().field, saved.field) != 27 || calls != 1) $fatal;
    if (saved.field.n != 27 || current.field != null) $fatal;
    $display("*-* All Finished *-*");
    $finish;
  end
endmodule
