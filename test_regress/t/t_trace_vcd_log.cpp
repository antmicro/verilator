// -*- mode: C++; c-file-style: "cc-mode" -*-
//
// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain.
// SPDX-FileCopyrightText: 2026 Antmicro
// SPDX-License-Identifier: CC0-1.0

#include <verilated.h>
#include <verilated_vcd_c.h>

#include <memory>

#include VM_PREFIX_INCLUDE

int main(int argc, char** argv) {
    Verilated::debug(0);
    Verilated::traceEverOn(true);
    Verilated::commandArgs(argc, argv);

    const std::unique_ptr<VM_PREFIX> top{new VM_PREFIX{"top"}};

    const std::unique_ptr<VerilatedVcdC> tfp{new VerilatedVcdC};
    top->trace(tfp.get(), 99);
    const uint32_t log = tfp->declLog("log");
    const uint32_t other = tfp->declLog("other_log");
    tfp->open(VL_STRINGIFY(TEST_OBJ_DIR) "/simlog.vcd");

    top->clk = 0;
    for (uint64_t time = 0; time < 10; ++time) {
        top->clk = !top->clk;
        top->eval();
        tfp->dump(time);
        if (time == 3) {
            tfp->log(log, "text with spaces");
        } else if (time == 5) {
            tfp->log(log, "two\nlines and a \\");
            tfp->log(other, "on the other log");
        }
    }

    tfp->close();
    top->final();
    printf("*-* All Finished *-*\n");
    return 0;
}
