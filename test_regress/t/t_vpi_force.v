// ======================================================================
// This file ONLY is placed under the Creative Commons Public Domain.
// SPDX-FileCopyrightText: 2026 Christian Hecken
// SPDX-License-Identifier: CC0-1.0
// ======================================================================

// verilog_format: off
`define stop $stop
`define checkh(gotv,expv) do if ((gotv) !== (expv)) begin $write("%%Error: %s:%0d:  got='h%x exp='h%x\n", `__FILE__,`__LINE__, (gotv), (expv)); `stop; end while(0);
`define checks(gotv,expv) do if ((gotv) != (expv)) begin $write("%%Error: %s:%0d:  got='%s' exp='%s'\n", `__FILE__,`__LINE__, (gotv), (expv)); `stop; end while(0);
`define checkr(gotv,expv) do if ((gotv) != (expv)) begin $write("%%Error: %s:%0d:  got=%f exp=%f\n", `__FILE__,`__LINE__, (gotv), (expv)); `stop; end while(0);

// verilog_format: on
`define STRINGIFY(x) `"x`"

`ifdef VERILATOR_COMMENTS
`define PUBLIC_FORCEABLE /*verilator public_flat_rw*/  /*verilator forceable*/
`else
`define PUBLIC_FORCEABLE
`endif

module t;

  reg clk;

  initial begin
    clk = 0;
    forever #2 clk = ~clk;
  end

  Test test (.clk(clk));


endmodule

module Test (
    input clk
);

`ifdef IVERILOG
`elsif USE_VPI_NOT_DPI
`ifdef VERILATOR
`systemc_header
  extern "C" int putString();
  extern "C" int tryInvalidPutOperations();
  extern "C" int putInertialDelay();
  extern "C" int checkInertialDelay();
  extern "C" int forceValues();
  extern "C" int releaseValues();
  extern "C" int releasePartiallyForcedValues();
  extern "C" int checkValuesForced();
  extern "C" int checkValuesPartiallyForced();
  extern "C" int checkValuesReleased();
  extern "C" int checkNonContinuousValuesForced();
  extern "C" int checkContinuousValuesReleased();
  extern "C" int checkNonContinuousValuesPartiallyForced();
`verilog
`endif
`else
`ifdef VERILATOR
  import "DPI-C" context function int putString();
  import "DPI-C" context function int tryInvalidPutOperations();
  import "DPI-C" context function int putInertialDelay();
  import "DPI-C" context function int checkInertialDelay();
`endif
  import "DPI-C" context function int forceValues();
  import "DPI-C" context function int releaseValues();
  import "DPI-C" context function int releasePartiallyForcedValues();
  import "DPI-C" context function int checkValuesPartiallyForced();
  import "DPI-C" context function int checkValuesForced();
  import "DPI-C" context function int checkValuesReleased();
  import "DPI-C" context function int checkNonContinuousValuesForced();
  import "DPI-C" context function int checkContinuousValuesReleased();
  import "DPI-C" context function int checkNonContinuousValuesPartiallyForced();
`endif

  // Verify that vpi_put_value still works for strings
  string        str1       /*verilator public_flat_rw*/; // std::string

  // Verify that EmitCSyms changes still allow for forceable, but not
  // public_flat_rw signals. This signal is only forced and checked in this
  // SystemVerilog testbench, but not through VPI.
  logic         nonPublic /*verilator forceable*/; // CData

  // Verify that vpi_put_value still works with vpiInertialDelay
  logic [ 31:0] delayed    `PUBLIC_FORCEABLE; // IData

  // Clocked signals

  // Force with vpiIntVal
  logic         onebit     `PUBLIC_FORCEABLE; // CData
  logic [ 31:0] intval     `PUBLIC_FORCEABLE; // IData

  // Force with vpiVectorVal
  logic [  7:0] vectorC    `PUBLIC_FORCEABLE; // CData
  logic [ 61:0] vectorQ    `PUBLIC_FORCEABLE; // QData
  logic [127:0] vectorW    `PUBLIC_FORCEABLE; // VlWide

  // Force with vpiRealVal
  real          real1      `PUBLIC_FORCEABLE; // double

  // Force with vpiStringVal
  logic [ 15:0] textHalf   `PUBLIC_FORCEABLE; // SData
  logic [ 63:0] textLong   `PUBLIC_FORCEABLE; // QData
  logic [511:0] text       `PUBLIC_FORCEABLE; // VlWide

  // Force with vpiBinStrVal, vpiOctStrVal, vpiHexStrVal
  logic [ 7:0]  binString  `PUBLIC_FORCEABLE; // CData
  logic [ 14:0] octString  `PUBLIC_FORCEABLE; // SData
  logic [ 63:0] hexString  `PUBLIC_FORCEABLE; // QData

  // Force with vpiDecStrVal
  logic [  7:0] decStringC `PUBLIC_FORCEABLE; // CData
  logic [ 15:0] decStringS `PUBLIC_FORCEABLE; // SData
  logic [ 31:0] decStringI `PUBLIC_FORCEABLE; // IData
  logic [ 63:0] decStringQ `PUBLIC_FORCEABLE; // QData

  // Continuously assigned signals:

  // Force with vpiIntVal
  wire         onebitContinuously     `PUBLIC_FORCEABLE; // CData
  wire [ 31:0] intvalContinuously     `PUBLIC_FORCEABLE; // IData

  // Force with vpiVectorVal
  wire [  7:0] vectorCContinuously    `PUBLIC_FORCEABLE; // CData
  wire [ 61:0] vectorQContinuously    `PUBLIC_FORCEABLE; // QData
  wire [127:0] vectorWContinuously    `PUBLIC_FORCEABLE; // VlWide

  // Force with vpiRealVal
  `ifdef IVERILOG
  // Need wreal with Icarus for forcing continuously assigned real
  wreal        real1Continuously       `PUBLIC_FORCEABLE; // double
  `else
  real         real1Continuously       `PUBLIC_FORCEABLE; // double
  `endif

  // Force with vpiStringVal
  wire [ 15:0] textHalfContinuously   `PUBLIC_FORCEABLE; // SData
  wire [ 63:0] textLongContinuously   `PUBLIC_FORCEABLE; // QData
  wire [511:0] textContinuously       `PUBLIC_FORCEABLE; // VlWide

  // Force with vpiBinStrVal, vpiOctStrVal, vpiHexStrVal
  wire [ 7:0]  binStringContinuously  `PUBLIC_FORCEABLE; // CData
  wire [ 14:0] octStringContinuously  `PUBLIC_FORCEABLE; // SData
  wire [ 63:0] hexStringContinuously  `PUBLIC_FORCEABLE; // QData

  // Force with vpiDecStrVal
  wire [  7:0] decStringCContinuously `PUBLIC_FORCEABLE; // CData
  wire [ 15:0] decStringSContinuously `PUBLIC_FORCEABLE; // SData
  wire [ 31:0] decStringIContinuously `PUBLIC_FORCEABLE; // IData
  wire [ 63:0] decStringQContinuously `PUBLIC_FORCEABLE; // QData

  initial begin
    void'($c32("forceValues()"));
    #1 `checkh(intval, 32'h55555555);
     $display("*-* All Finished *-*");
     $finish;
  end
endmodule
