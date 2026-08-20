// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain.
// SPDX-FileCopyrightText: 2026 Antmicro
// SPDX-License-Identifier: CC0-1.0

`define stop $stop
// verilog_format: off
`define checkd(gotv, expv) \
  do if ((gotv) !== (expv)) begin \
    $write("%%Error: %s:%0d: got=%0d exp=%0d\n", `__FILE__, `__LINE__, (gotv), (expv)); \
    `stop; \
  end while (0)
// verilog_format: on

module t;
  int x;
  int action_count;
  int fail_count;
  int pass_count;
  int fail_arg;
  int pass_arg;
  int late_global;
  int global_value;
  int side_count;
  int multi_count;
  int multi_sum;
  int ref_value;
  int ref_seen;
  int const_ref_seen;
  int static_ref_seen;
  int delayed_count;
  int default_seen;
  int cross_process_count;
  string sformat_text;
  event cross_process_flush;

  initial begin
    x = 0;
    global_value = 4;
    assert #0 (x != 0) record_pass(x);
    else record_fail(x);
    x = 1;
    `checkd(action_count, 0);
    global_value = 8;
    assert #0 (1) record_pass(side(x));
    `checkd(side_count, 1);
    x = 20;

    for (int i = 0; i < 3; i++) begin
      x = i + 1;
      assert #0 (1) record_multi(x);
    end
    `checkd(multi_count, 0);

    x = 7;
    assert #0 (1) $sformat(sformat_text, "%0d", side(x));
    `checkd(side_count, 2);
    assert #0 (1) $fdisplay(32'h80000001, "%0d", x);
    ref_value = 12;
    assert #0 (1) record_ref(ref_value);
    assert #0 (1) record_const_ref(ref_value);
    assert #0 (1) record_default();
    ref_static();
    ref_value = 13;

    #1;
    `checkd(action_count, 2);
    `checkd(fail_count, 1);
    `checkd(fail_arg, 0);
    `checkd(pass_count, 1);
    `checkd(pass_arg, 11);
    `checkd(late_global, 8);
    `checkd(multi_count, 3);
    `checkd(multi_sum, 6);
    `checkd(sformat_text.atoi(), 17);
    `checkd(ref_seen, 13);
    `checkd(const_ref_seen, 13);
    `checkd(static_ref_seen, 22);
    `checkd(default_seen, 9);

    #1 assert #0 (1) record_delayed();
    `checkd(delayed_count, 0);
    #1;
    `checkd(delayed_count, 1);

    #2;
    $write("*-* All Finished *-*\n");
    $finish;
  end

  initial begin
    #3;
    assert #0 (1) record_cross_process();
    ->cross_process_flush;
    #1;
    `checkd(cross_process_count, 1);
  end

  initial begin
    @cross_process_flush;
  end

  function int side(input int v);
    side_count++;
    return v + 10;
  endfunction

  task record_fail(input int snap);
    action_count++;
    fail_count++;
    fail_arg = snap;
    late_global = global_value;
  endtask

  task record_pass(input int snap);
    action_count++;
    pass_count++;
    pass_arg = snap;
    late_global = global_value;
  endtask

  task record_multi(input int snap);
    multi_count++;
    multi_sum += snap;
  endtask

  task record_ref(ref int snap);
    ref_seen = snap;
  endtask

  task record_const_ref(const ref int snap);
    const_ref_seen = snap;
  endtask

  task record_static_ref(ref int snap);
    static_ref_seen = snap;
  endtask

  task record_default(input int snap = 9);
    default_seen = snap;
  endtask

  task record_cross_process();
    cross_process_count++;
  endtask

  task record_delayed();
    delayed_count++;
  endtask

  task ref_static();
    static int a;
    a = 21;
    assert #0 (1) record_static_ref(a);
    a = 22;
  endtask

endmodule
