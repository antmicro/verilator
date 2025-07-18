// -*- SystemC -*-
// DESCRIPTION: Verilator Example: Top level main for invoking SystemC model
//
// This file ONLY is placed under the Creative Commons Public Domain, for
// any use, without warranty, 2017 by Wilson Snyder.
// SPDX-License-Identifier: CC0-1.0
//======================================================================

// For std::unique_ptr
#include <memory>

// SystemC global header
#include <systemc>
#include <type_traits>

// Include common routines
#include <verilated.h>
#include <verilated_sc_trace_ext.h>
#include <verilated_vcd_c.h>
#include <verilated_vcd_sc.h>

#include <sys/stat.h>  // mkdir

// Include model header, generated from Verilating "top.v"
#include "Vtop.h"

using namespace sc_core;
using namespace sc_dt;

int sc_main(int argc, char* argv[]) {
    // This is a more complicated example, please also see the simpler examples/make_hello_c.

    // Create logs/ directory in case we have traces to put under it
    Verilated::mkdir("logs");

    // Set debug level, 0 is off, 9 is highest presently used
    // May be overridden by commandArgs argument parsing
    Verilated::debug(0);

    // Before any evaluation, need to know to calculate those signals only used for tracing
    Verilated::traceEverOn(true);

    // Pass arguments so Verilated code can see them, e.g. $value$plusargs
    // This needs to be called before you create any model
    Verilated::commandArgs(argc, argv);

    // General logfile
    std::ios::sync_with_stdio();

    // Define a clock
    sc_clock clk{"clk", 1, SC_NS, 0.5, 3, SC_NS, true};

    // Define a custom signal to trace
    sc_signal<bool> bit_signal_to_trace{"bit_signal_to_trace_sc", true};
    sc_signal<bool> bit_signal_to_trace2{"bit_signal_to_trace_sc2", false};
    sc_signal<CData> int_signal_to_trace{"int_signal_to_trace_sc", 12};
    sc_signal<CData> int_signal_to_trace2{"int_signal_to_trace_sc2", 123};

    // Construct the Verilated model
    const std::unique_ptr<Vtop> top = std::make_unique<Vtop>("top");

    // Attach Vtop's signals to this upper model
    top->clk(clk);

    // You must do one evaluation before enabling waves, in order to allow
    // SystemC to interconnect everything for testing.
    sc_start(SC_ZERO_TIME);

    std::cout << "Enabling waves into logs/vlt_dump.vcd...\n";
    std::unique_ptr<VerilatedVcdSc> tfp = std::make_unique<VerilatedVcdSc>();

    auto bitSignal = makeScSignalExternalVariable(bit_signal_to_trace, *tfp);
    auto bitSignal2 = makeScSignalExternalVariable(bit_signal_to_trace2, *tfp);
    auto intSignal = makeScSignalExternalVariableCData(int_signal_to_trace, *tfp);
    auto intSignal2 = makeScSignalExternalVariableCData(int_signal_to_trace2, *tfp);

    tfp->spTrace()->addTraceVar(&intSignal);
    tfp->spTrace()->addTraceVar(&intSignal2);
    tfp->spTrace()->addTraceVar(&bitSignal);
    tfp->spTrace()->addTraceVar(&bitSignal2);

    top->trace(tfp.get(), 99);  // Trace 99 levels of hierarchy
    Verilated::mkdir("logs");
    tfp->open("logs/vlt_dump.vcd");

    // Simulate until $finish
    while (!Verilated::gotFinish()) {
        // Flush the wave files each cycle so we can immediately see the output
        // Don't do this in "real" programs, do it in an abort() handler instead
        if (tfp) tfp->flush();

        // Change external signals
        bit_signal_to_trace = !bit_signal_to_trace;
        bit_signal_to_trace2 = !bit_signal_to_trace2;
        int_signal_to_trace = int_signal_to_trace + 1;
        int_signal_to_trace2 = int_signal_to_trace2 - 2;

        std::printf(
            "[%f] bit_signal_to_trace=%hhu int_signal_to_trace=%hhu bit_signal_to_trace2=%hhu\n",
            sc_core::sc_time_stamp().to_double(), bitSignal.val(), intSignal.val(),
            bitSignal2.val());

        // Simulate 1ns
        sc_start(1, SC_NS);
    }

    // Final model cleanup
    top->final();

    return 0;
}
