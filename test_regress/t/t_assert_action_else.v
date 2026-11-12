// DESCRIPTION: Verilator: Bind else to the enclosing if after an assertion with a null action
//
// This file ONLY is placed under the Creative Commons Public Domain.
// SPDX-FileCopyrightText: 2026 The Verilator Authors
// SPDX-License-Identifier: CC0-1.0

// verilog_format: off
`define stop $stop
`define checkd(gotv,expv) do if ((gotv) !== (expv)) begin $write("%%Error: %s:%0d:  got=%0d exp=%0d\n", `__FILE__,`__LINE__, (gotv), (expv)); `stop; end while(0);
// verilog_format: on

module t;
  function automatic bit count_call(ref int calls);
    ++calls;
    return 1;
  endfunction

  initial begin
    int when_true, when_false;
    int fail_only, passed, failed, skipped;

    // verilog_format: off
    for (int i = 0; i < 4; ++i) begin
      // The semicolon finishes the first assertion. The else belongs to the if.
      if (i[0])
        assert (count_call(when_true));
      else
        assert (count_call(when_false));

      // An assertion can have a failure action alone, or both success and failure actions.
      assert (i[0]) else ++fail_only;
      if (i[1])
        assert (i[0]) ++passed; else ++failed;
      else
        ++skipped;
    end
    // verilog_format: on
    `checkd(when_true, 2);
    `checkd(when_false, 2);
    `checkd(fail_only, 2);
    `checkd(passed, 1);
    `checkd(failed, 1);
    `checkd(skipped, 2);
    $write("*-* All Finished *-*\n");
    $finish;
  end
endmodule
