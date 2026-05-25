module t #(
) (
);
  logic a;
  logic [6:0] b;
  logic c;
  assign c = ({a, b} == 8'hFC);

  initial begin
    $write("*-* All Finished *-*\n");
    $finish;
  end
endmodule
