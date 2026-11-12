// DESCRIPTION: Verilator: Conditional generate hierarchical references
// SPDX-FileCopyrightText: 2026 The Verilator Authors
// SPDX-License-Identifier: CC0-1.0

module target #(parameter int Value = 0);
  if (1) begin : port
    task automatic get(output int data);
      data = Value;
    endtask
  end
endmodule

module harness #(parameter bit Mode = 0) (output int result);
  if (Mode) begin : access
    initial chosen_a[0].port.get(result);
  end else begin : access
    initial chosen_b[0].port.get(result);
  end
  case (Mode)
    0: begin : extra
      int check;
      initial begin
        chosen_b[0].port.get(check);
        if (check != 72) $fatal;
      end
    end
    1: begin : extra
    end
    default: begin : extra
      initial chosen_b[0].nonexistent.get(result);
    end
  endcase
endmodule

module parent_a(output int data);
  target #(.Value(29)) chosen_a[0:0] ();
  harness #(.Mode(1)) h (.result(data));
endmodule

module parent_b(output int data);
  target #(.Value(72)) chosen_b[0:0] ();
  harness #(.Mode(0)) h (.result(data));
endmodule

module t (
    input clk
);
  int a, b;
  parent_a u_a (.data(a));
  parent_b u_b (.data(b));
  always @(posedge clk) begin
    if (a != 29 || b != 72) $fatal(1, "got %0d %0d", a, b);
    $display("*-* All Finished *-*");
    $finish;
  end
endmodule
