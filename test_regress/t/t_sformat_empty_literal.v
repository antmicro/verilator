// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain.
// SPDX-FileCopyrightText: 2026 Wilson Snyder
// SPDX-License-Identifier: CC0-1.0

module t;
  string fmt;
  string result;

  initial begin
    result = $sformatf("%s%s%s", "", "middle", "");
    if (result != "middle") $stop;

    if (!$value$plusargs("fmt=%s", fmt)) fmt = "[%s]%s[%0s]";
    result = $sformatf(fmt, "", "middle", "\000");
    if (result != "[]middle[]") $stop;

    if (!$value$plusargs("numfmt=%s", fmt)) fmt = "%h %0d";
    result = $sformatf(fmt, "", "");
    if (result != "00 0") $stop;

    $display("*-* All Finished *-*");
    $finish;
  end
endmodule
