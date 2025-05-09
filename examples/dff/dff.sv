primitive altos_dff_sr_err (q, clk, d, s, r);
	output q;
	reg q;
	input clk, d, s, r;

	table
		 ?   1 (0x)  ?   : ? : -;
		 ?   0  ?   (0x) : ? : -;
		 ?   0  ?   (x0) : ? : -;
		(0x) ?  0    0   : ? : 0;
		(0x) 1  x    0   : ? : 0;
		(0x) 0  0    x   : ? : 0;
		(1x) ?  0    0   : ? : 1;
		(1x) 1  x    0   : ? : 1;
		(1x) 0  0    x   : ? : 1;
	endtable
endprimitive

primitive altos_dff_sr_0 (q, v, clk, d, s, r, xcr);
	output q;
	reg q;
	input v, clk, d, s, r, xcr;

	table
	//	v, clk, d, s, r, xcr : q' : q;

		*  ?   ?   ?   ?   ? : ? : x;
		?  ?   ?   ?   1   ? : ? : 0;
		?  ?   ?   1   0   ? : ? : 1;
		?  b   ? (1?)  0   ? : 1 : -;
		?  x   1 (1?)  0   ? : 1 : -;
		?  ?   ? (10)  0   ? : ? : -;
		?  ?   ? (x0)  0   ? : ? : -;
		?  ?   ? (0x)  0   ? : 1 : -;
		?  b   ?  0   (1?) ? : 0 : -;
		?  x   0  0   (1?) ? : 0 : -;
		?  ?   ?  0   (10) ? : ? : -;
		?  ?   ?  0   (x0) ? : ? : -;
		?  ?   ?  0   (0x) ? : 0 : -;
		? (x1) 0  0    ?   0 : ? : 0;
		? (x1) 1  ?    0   0 : ? : 1;
		? (x1) 0  0    ?   1 : 0 : 0;
		? (x1) 1  ?    0   1 : 1 : 1;
		? (x1) ?  ?    0   x : ? : -;
		? (x1) ?  0    ?   x : ? : -;
		? (1x) 0  0    ?   ? : 0 : -;
		? (1x) 1  ?    0   ? : 1 : -;
		? (x0) 0  0    ?   ? : ? : -;
		? (x0) 1  ?    0   ? : ? : -;
		? (x0) ?  0    0   x : ? : -;
		? (0x) 0  0    ?   ? : 0 : -;
		? (0x) 1  ?    0   ? : 1 : -;
		? (01) 0  0    ?   ? : ? : 0;
		? (01) 1  ?    0   ? : ? : 1;
		? (10) ?  0    ?   ? : ? : -;
		? (10) ?  ?    0   ? : ? : -;
		?  b   *  0    ?   ? : ? : -;
		?  b   *  ?    0   ? : ? : -;
		?  ?   ?  ?    ?   * : ? : -;
	endtable
endprimitive

`timescale 1ns/10ps
`celldefine
module DFFASRHQNx1_ASAP7_75t_R (QN, D, RESETN, SETN, CLK);
	output QN;
	input D, RESETN, SETN, CLK;
	reg notifier;
	wire delayed_D, delayed_RESETN, delayed_SETN, delayed_CLK;

	// Function
	wire int_fwire_d, int_fwire_IQN, int_fwire_r;
	wire int_fwire_s, xcr_0;

	not (int_fwire_d, delayed_D);
	not (int_fwire_s, delayed_RESETN);
	not (int_fwire_r, delayed_SETN);
	altos_dff_sr_err (xcr_0, CLK, int_fwire_d, int_fwire_s, int_fwire_r);
	altos_dff_sr_0 (int_fwire_IQN, notifier, CLK, int_fwire_d, int_fwire_s, int_fwire_r, xcr_0);
	buf (QN, int_fwire_IQN);

	// Timing

	// Additional timing wires
	wire adacond0, adacond1, adacond2;
	wire adacond3, adacond4, adacond5;
	wire adacond6, adacond7, adacond8;
	wire CLK__bar, D__bar;


	// Additional timing gates
	and (adacond0, RESETN, SETN);
	and (adacond1, D, SETN);
	and (adacond2, CLK, SETN);
	not (CLK__bar, CLK);
	and (adacond3, CLK__bar, SETN);
	not (D__bar, D);
	and (adacond4, D__bar, RESETN);
	and (adacond5, CLK, RESETN);
	and (adacond6, CLK__bar, RESETN);
	and (adacond7, D, RESETN, SETN);
	and (adacond8, D__bar, RESETN, SETN);

	specify
		if ((CLK & SETN))
			(negedge RESETN => (QN+:1'b1)) = 0;
		if ((~CLK & D & SETN))
			(negedge RESETN => (QN+:1'b1)) = 0;
		if ((~CLK & ~D & SETN))
			(negedge RESETN => (QN+:1'b1)) = 0;
		ifnone (negedge RESETN => (QN+:1'b1)) = 0;
		if ((CLK & RESETN))
			(negedge SETN => (QN+:1'b0)) = 0;
		if ((CLK & ~RESETN))
			(negedge SETN => (QN+:1'b0)) = 0;
		if ((~CLK & D & RESETN))
			(negedge SETN => (QN+:1'b0)) = 0;
		if ((~CLK & D & ~RESETN))
			(negedge SETN => (QN+:1'b0)) = 0;
		if ((~CLK & ~D & RESETN))
			(negedge SETN => (QN+:1'b0)) = 0;
		if ((~CLK & ~D & ~RESETN))
			(negedge SETN => (QN+:1'b0)) = 0;
		ifnone (negedge SETN => (QN+:1'b0)) = 0;
		if ((CLK & ~RESETN))
			(posedge SETN => (QN+:1'b1)) = 0;
		if ((~CLK & D & ~RESETN))
			(posedge SETN => (QN+:1'b1)) = 0;
		if ((~CLK & ~D & ~RESETN))
			(posedge SETN => (QN+:1'b1)) = 0;
		ifnone (posedge SETN => (QN+:1'b1)) = 0;
		(posedge CLK => (QN+:!D)) = 0;
		$setuphold (posedge CLK &&& adacond0, posedge D &&& adacond0, 0, 0, notifier,,, delayed_CLK, delayed_D);
		$setuphold (posedge CLK &&& adacond0, negedge D &&& adacond0, 0, 0, notifier,,, delayed_CLK, delayed_D);
		$setuphold (posedge CLK, posedge D, 0, 0, notifier,,, delayed_CLK, delayed_D);
		$setuphold (posedge CLK, negedge D, 0, 0, notifier,,, delayed_CLK, delayed_D);
		$setuphold (posedge SETN &&& CLK, posedge RESETN &&& CLK, 0, 0, notifier,,, delayed_SETN, delayed_RESETN);
		$setuphold (posedge SETN &&& ~CLK, posedge RESETN &&& ~CLK, 0, 0, notifier,,, delayed_SETN, delayed_RESETN);
		$setuphold (posedge SETN, posedge RESETN, 0, 0, notifier,,, delayed_SETN, delayed_RESETN);
		$setuphold (posedge RESETN &&& CLK, posedge SETN &&& CLK, 0, 0, notifier,,, delayed_RESETN, delayed_SETN);
		$setuphold (posedge RESETN &&& ~CLK, posedge SETN &&& ~CLK, 0, 0, notifier,,, delayed_RESETN, delayed_SETN);
		$setuphold (posedge RESETN, posedge SETN, 0, 0, notifier,,, delayed_RESETN, delayed_SETN);
		$recovery (posedge RESETN &&& adacond1, posedge CLK &&& adacond1, 0, notifier);
		$recovery (posedge RESETN, posedge CLK, 0, notifier);
		$hold (posedge CLK &&& adacond1, posedge RESETN &&& adacond1, 0, notifier);
		$hold (posedge CLK, posedge RESETN, 0, notifier);
		$recovery (posedge SETN &&& adacond4, posedge CLK &&& adacond4, 0, notifier);
		$recovery (posedge SETN, posedge CLK, 0, notifier);
		$hold (posedge CLK &&& adacond4, posedge SETN &&& adacond4, 0, notifier);
		$hold (posedge CLK, posedge SETN, 0, notifier);
		$width (negedge RESETN &&& adacond2, 0, 0, notifier);
		$width (negedge RESETN &&& adacond3, 0, 0, notifier);
		$width (negedge SETN &&& adacond5, 0, 0, notifier);
		$width (negedge SETN &&& adacond6, 0, 0, notifier);
		$width (posedge CLK &&& adacond7, 0, 0, notifier);
		$width (negedge CLK &&& adacond7, 0, 0, notifier);
		$width (posedge CLK &&& adacond8, 0, 0, notifier);
		$width (negedge CLK &&& adacond8, 0, 0, notifier);
	endspecify
endmodule
`endcelldefine
 
module t;
    reg CLK = 0;
    reg D = 0;
    reg RESETN = 0;
    reg SETN = 1;
    reg Q1, Q2, Q3;
    always #5 CLK = ~CLK;
    always #20 D = ~D;
    always #15 SETN = ~SETN;
    always #11 RESETN = ~RESETN;

    always @(posedge CLK) Q2 <= ~D;
    always @(posedge CLK) Q3 = ~D;
    
    // `ifndef VERILATOR
    //     always @(edge Q3) #0 $display("Inactive");
    // `endif

    DFFASRHQNx1_ASAP7_75t_R dff (.QN(Q1), .*);

	always @(edge RESETN)   $display("%0t RESETN = ~RESETN        | D=%b Q1=%b Q2=%b Q3=%b SETN=%b RESETN=%b", $time, D, Q1, Q2, Q3, SETN, RESETN);
	always @(edge SETN)     $display("%0t SETN = ~SETN            | D=%b Q1=%b Q2=%b Q3=%b SETN=%b RESETN=%b", $time, D, Q1, Q2, Q3, SETN, RESETN);
    always @(edge Q1)       $display("%0t DFFHQNx1_ASAP7_75t_R    | D=%b Q1=%b Q2=%b Q3=%b SETN=%b RESETN=%b", $time, D, Q1, Q2, Q3, SETN, RESETN);
    always @(edge Q2)       $display("%0t @(posedge CLK) Q2 <= ~D | D=%b Q1=%b Q2=%b Q3=%b SETN=%b RESETN=%b", $time, D, Q1, Q2, Q3, SETN, RESETN);
    always @(edge Q3)       $display("%0t @(posedge CLK) Q3 = ~D  | D=%b Q1=%b Q2=%b Q3=%b SETN=%b RESETN=%b", $time, D, Q1, Q2, Q3, SETN, RESETN);

	always @(negedge CLK)
		$strobe("%0t", $time);

    initial begin
        $dumpfile("dff.vcd");
        $dumpvars;
        #200 $finish;
    end
endmodule