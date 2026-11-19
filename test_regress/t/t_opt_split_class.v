// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain.
// SPDX-FileCopyrightText: 2026 Wilson Snyder
// SPDX-License-Identifier: CC0-1.0

// verilog_format: off
`define stop $stop
`define checkd(gotv,expv) do if ((gotv) !== (expv)) begin $write("%%Error: %s:%0d:  got=%0d exp=%0d\n", `__FILE__,`__LINE__, (gotv), (expv)); `stop; end while(0);
// verilog_format: on

class Worker;
  int total;
  int values[];
  int history[$];

  function new(input int seed);
    values = new[3];
    total = seed;
  endfunction

  function int run(input int n, const ref int bias, ref int data[3:1]);
    int acc = n;
    int tmp[3:1] = '{default:0};
    string text = "start";
    if (n > 0) begin
      for (int i = 0; i < 8; i++) begin
        int adjust;
        if (i == 1 && n[0]) continue;
        if (i == 4 && n[1]) return acc + bias + tmp[1];
        acc = acc * 3 + n + i;
        tmp[1] += acc;
        tmp[2] = tmp[1] + bias;
        tmp[3] = tmp[2] - n;
        values[0] = tmp[1];
        values[1] = tmp[2];
        values[2] = tmp[3];
        history.push_back(acc);
        text = {text, "x"};
        total = tmp[1] + tmp[2] + tmp[3] + text.len();
        data[1] += tmp[1];
        data[2] ^= tmp[2];
        data[3] = tmp[3];
        adjust = acc + i;
        acc = adjust * 2 - bias;
        if (i == 5) break;
      end
    end
    return acc + bias + tmp[1];
  endfunction

  function int value(input int n);
    int acc = n;
    if (n > 0) begin
      acc += 2;
      acc *= 3;
      acc -= n;
      acc ^= 7;
      acc += n;
      acc *= 2;
      acc -= 3;
      acc += n;
      acc ^= 5;
      acc += n;
    end
    return acc;
  endfunction
endclass

module t;
  initial begin
    automatic int bias = 7;
    int data[3:1];
    int returned;
    Worker worker;
    automatic int expected[6][10] = '{
      '{3650, 4121, 1650, 1435, 1372, 5, 1366, 1373, 1372, 28},
      '{3243, 3672, 1470, 1084, 1222, 4, 1217, 1224, 1222, 26},
      '{1073, 1222, 481, 448, 405, 3, 401, 408, 405, 28},
      '{325468, 366159, 146456, 105654, 122049, 6, 122046, 122053, 122049, 34},
      '{73314, 82477, 32970, 30995, 27488, 5, 27486, 27493, 27488, 68},
      '{14847, 16712, 6670, 5852, 5566, 4, 5565, 5572, 5566, 66}
    };
    for (int n = 1; n <= 6; n++) begin
      worker = new(n);
      data = '{default:0};
      returned = worker.run(n, bias, data);
      `checkd(returned, expected[n-1][0]);
      `checkd(worker.total, expected[n-1][1]);
      `checkd(data[1], expected[n-1][2]);
      `checkd(data[2], expected[n-1][3]);
      `checkd(data[3], expected[n-1][4]);
      `checkd(worker.history.size(), expected[n-1][5]);
      `checkd(worker.values[0], expected[n-1][6]);
      `checkd(worker.values[1], expected[n-1][7]);
      `checkd(worker.values[2], expected[n-1][8]);
      `checkd(worker.value(n), expected[n-1][9]);
    end
    $write("*-* All Finished *-*\n");
    $finish;
  end
endmodule
