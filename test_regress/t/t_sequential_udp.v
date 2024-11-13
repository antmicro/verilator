// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed into the Public Domain, for any use,
// without warranty, 2015 by Mike Thyer.
// SPDX-License-Identifier: CC0-1.0

primitive d_edge_ff (q, clock, data);
    output q; reg q;
    input clock, data;
    initial q = 1'b1;
    table
        // clock data q q+
        // obtain output on rising edge of clock
        (01) 0 : ? : 0 ;
        (01) 1 : ? : 1 ;
        (0?) 1 : 1 : 1 ;
        (0?) 0 : 0 : 0 ;
        // ignore negative edge of clock
        (?0) ? : ? :- ;
        // ignore data changes on steady clock
        ? (??) : ? :- ;
    endtable
endprimitive
module t ();
    reg qi, q, d, clk;
    d_edge_ff   g1 (qi, clk, d);
    buf #3 g2 (q, qi);
    initial begin
        if (q != 0 || qi != 1) $stop;
        #5
        if (q != 1 || qi != 1) $stop;
        $write("*-* All Finished *-*\n");
        $finish;
    end
 endmodule
