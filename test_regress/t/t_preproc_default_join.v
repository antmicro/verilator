// DESCRIPTION: Verilator: SystemVerilog regression test
// SPDX-FileCopyrightText: 2026 The Verilator Authors
// SPDX-License-Identifier: CC0-1.0

// verilog_format: off
`define report_error(value) value += 1
`define report_warning(value) value += 7
`define report(value, severity = error ) `report_``severity(value)
`define ident(root = result, tail = field ) root``_``tail
`define spaced(text = " two words " ) text
// verilog_format: on

module t;
  int result_field;
  initial begin
    `report(result_field);
    `report(result_field, warning);
    `report(result_field,);
    if (`ident() != 9 || `spaced() != " two words ") $fatal;
    $display("*-* All Finished *-*");
    $finish;
  end
endmodule
