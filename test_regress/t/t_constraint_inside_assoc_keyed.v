// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain.
// SPDX-FileCopyrightText: 2026 The Verilator Authors
// SPDX-License-Identifier: CC0-1.0

// verilog_format: off
`define stop $stop
`define checkd(gotv,expv) do if ((gotv) !== (expv)) begin $write("%%Error: %s:%0d:  got=%0d exp=%0d\n", `__FILE__,`__LINE__, (gotv), (expv)); `stop; end while(0);
// verilog_format: on

module t;
  typedef bit [96:0] index_t;

  class Picker;
    bit [6:0] allowed[string];
    bit [6:0] excluded[index_t];
    bit [6:0] active[*];
    rand bit [6:0] choice;
    constraint select_value {
      choice inside {allowed};
      choice inside {active};
      !(choice inside {excluded});
    }
  endclass

  class Joint;
    rand bit [6:0] entries[index_t];
    rand bit [6:0] choice;
    bit [6:0] expected;
    constraint match_value {
      choice == expected;
      choice inside {entries};
    }
  endclass

  initial begin
    Picker picker;
    Joint joint;
    bit [8:0] probe;
    int success;
    picker = new;
    picker.allowed["left"] = 7'd17;
    picker.allowed["right"] = 7'd41;
    picker.active[8'h07] = 7'd17;
    picker.active[{1'b1, 96'd7}] = 7'd41;
    picker.excluded[97'd6] = 7'd17;

    // The constraints leave exactly one value after each update.
    success = picker.randomize();
    `checkd(success, 1);
    `checkd(picker.choice, 7'd41);
    picker.excluded.delete();
    picker.excluded[{1'b1, 96'd6}] = 7'd41;
    success = picker.randomize();
    `checkd(success, 1);
    `checkd(picker.choice, 7'd17);

    // A scope randomize must compare the full operand against the present values.
    success = std::randomize(
        probe
    ) with {
      probe == 9'h111;
      !(probe inside {picker.allowed});
    };
    `checkd(success, 1);
    `checkd(probe, 9'h111);

    // The sole present entry must change with the jointly solved choice, even after rekeying.
    joint = new;
    joint.entries[8'd6] = 7'd3;
    joint.expected = 7'd44;
    success = joint.randomize();
    `checkd(success, 1);
    `checkd(joint.choice, 7'd44);
    `checkd(joint.entries[8'd6], 7'd44);
    joint.entries.delete(8'd6);
    joint.entries[{1'b1, 46'd6}] = 7'd3;
    joint.expected = 7'd73;
    success = joint.randomize();
    `checkd(success, 1);
    `checkd(joint.choice, 7'd73);
    `checkd(joint.entries[{1'b1, 46'd6}], 7'd73);
    `checkd(joint.entries.exists(8'd6), 0);
    `checkd(joint.entries.num(), 1);

    $write("*-* All Finished *-*\n");
    $finish;
  end
endmodule
