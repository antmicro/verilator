module tb();
    logic clk = 0;
    logic x;
    logic y;

    always #1ns clk = ~clk;

    assign y = x;

    default clocking cb @(posedge clk);
        output #1ns x;
        input #1step y;
    endclocking

    initial begin
        cb.x <= '0;
        ##2;
        fork
            begin
                repeat(5) begin
                    cb.x <= ~cb.y;
                    @cb;
                end
            end
            begin
                repeat(4) begin
                    ##1;
                end
            end
        join_any
        disable fork;
        $finish();
    end
endmodule
