// -*- mode: C++; c-file-style: "cc-mode" -*-
//
// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain.
// SPDX-FileCopyrightText: 2026 Wilson Snyder
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
    tfp->open(VL_STRINGIFY(TEST_OBJ_DIR) "/simcomment.vcd");

    tfp->comment("comment before first dump");

    top->clk = 0;
    for (uint64_t time = 0; time < 10; ++time) {
        top->clk = !top->clk;
        top->eval();
        tfp->dump(time);
        if (time == 3) {
            tfp->comment("single line comment at t=3");
        } else if (time == 5) {
            tfp->comment("multi line comment at t=5\nsecond line");
        }
    }

    tfp->close();
    top->final();
    printf("*-* All Finished *-*\n");
    return 0;
}
