// DESCRIPTION: Verilator: Direct captures of lexical module owners and descendants
//
// This file ONLY is placed under the Creative Commons Public Domain.
// SPDX-FileCopyrightText: 2026 Wilson Snyder
// SPDX-License-Identifier: CC0-1.0

// Check that class methods retain the correct module owner across hierarchy levels.
// Exercise live reads, constructor captures, inherited methods and named-block lookup.

// verilog_format: off
`define stop $stop
`define checkd(gotv,expv) do if ((gotv) !== (expv)) begin $write("%%Error: %s:%0d: %m got=%0d exp=%0d\n", `__FILE__,`__LINE__, (gotv), (expv)); `stop; end while(0)
// verilog_format: on

module scope_grandchild;
  int y;
endmodule

module scope_child;
  int y;
  scope_grandchild child ();
endmodule

module scope_owner;
  int x;
  scope_child child ();
  class LocalReader;
    int value;
    function int get();
      // Read the declaring module's live state through its owner capture.
      return x;
    endfunction
    function int get_value();
      return value;
    endfunction
  endclass
  class Reader;
    // Each method captures its target scope directly, even two levels below the owner.
    function int get();
      return child.y;
    endfunction
    function int get_grandchild();
      return child.child.y;
    endfunction
  endclass
  class Derived extends Reader;
  endclass
  class Capture;
    int captured;
    function new();
      // The owner capture must be usable before the constructor body runs.
      captured = get();
    endfunction
    function int get();
      return child.y;
    endfunction
  endclass
  Reader direct = new;
  LocalReader local_reader = new;
  Capture captured;
  Derived inherited = new;
endmodule

module context_leaf #(
    parameter int ID = 1
) (
    output bit done
);
  scope_owner scope ();

  task automatic check_state(int delta);
    `checkd(scope.local_reader.get(), ID + delta);
    `checkd(scope.local_reader.get_value(), ID + 200 + delta);
    `checkd(scope.direct.get(), ID + delta);
    `checkd(scope.direct.get_grandchild(), ID + 10 + delta);
    `checkd(scope.inherited.get(), ID + delta);
    // Live reads change with the owner; the constructor's saved value stays fixed.
    `checkd(scope.captured.get(), ID + delta);
    `checkd(scope.captured.captured, ID);
  endtask

  initial begin
    scope.x = ID;
    scope.child.y = ID;
    scope.child.child.y = ID + 10;
    scope.captured = new;
    #1;
    scope.local_reader.value = ID + 200;
    #1;
    check_state(0);
    // Mutate owner and object state separately to expose a wrongly bound receiver.
    scope.x = ID + 100;
    scope.local_reader.value = ID + 300;
    scope.child.y = ID + 100;
    scope.child.child.y = ID + 110;
    #1;
    check_state(100);
    done = 1;
  end
endmodule

module context_chain #(
    parameter int ID = 1
) (
    output wire done
);
  context_leaf #(ID) leaf (done);
endmodule

module block_counter (
    input logic clk,
    input int step,
    output int result
);
  // Preserve module boundaries to check ordinary lookup inside named blocks.
  /* verilator no_inline_module */
  int count;
  function automatic int advance(input int value);
    return value + step;
  endfunction
  initial begin : initialize_count
    count = 0;
  end
  always @(posedge clk) begin : update_count
    count <= advance(count);
  end
  always_comb begin : compute_result
    result = count ^ step;
  end
endmodule

module block_case (
    output bit done
);
  logic clk = 0;
  int result1, result2;
  block_counter
      first (
          clk,
          1,
          result1
      ),
      second (
          clk,
          3,
          result2
      );
  initial begin
    repeat (2) begin
      #5 clk = 1;
      #5 clk = 0;
    end
    `checkd(first.count, 2);
    `checkd(result1, 3);
    `checkd(second.count, 6);
    `checkd(result2, 5);
    done = 1;
  end
endmodule

module t;
  wire [3:0] done;
  // Reuse the same owner declaration at different depths with distinct live values.
  context_leaf #(1) flat_first (done[0]);
  context_leaf #(2) flat_second (done[1]);
  context_chain #(3) linear (done[2]);
  block_case blocks (done[3]);

  initial begin
    wait (&done);
    $write("*-* All Finished *-*\n");
    $finish;
  end
  initial begin
    #100;
    $error("Incomplete scenarios: done=%b", done);
    `stop;
  end
endmodule
