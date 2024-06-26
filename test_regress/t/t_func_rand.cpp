// -*- mode: C++; c-file-style: "cc-mode" -*-
//
// DESCRIPTION: Verilator: Verilog Test module
//
// This file ONLY is placed under the Creative Commons Public Domain, for
// any use, without warranty, 2006 by Wilson Snyder.
// SPDX-License-Identifier: CC0-1.0

#include <verilated.h>

#include VM_PREFIX_INCLUDE

double sc_time_stamp() { return 0; }

int main(int argc, char* argv[]) {
    Verilated::debug(0);
    Verilated::commandArgs(argc, argv);

    VM_PREFIX* topp = new VM_PREFIX;

    printf("\nTesting\n");
    for (int i = 0; i < 10; i++) {
        topp->clk = 0;
        topp->eval();
        topp->clk = 1;
        topp->eval();
    }
    if (topp->Rand != 0xfeed0fad) {
        vl_fatal(__FILE__, __LINE__, "top", "Unexpected value for Rand output\n");
    }
    topp->final();
    VL_DO_DANGLING(delete topp, topp);
    printf("*-* All Finished *-*\n");
}
