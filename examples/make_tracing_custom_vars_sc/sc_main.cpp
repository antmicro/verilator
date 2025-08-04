// -*- SystemC -*-
// DESCRIPTION: Verilator Example: Top level main for invoking SystemC model
//
// This file ONLY is placed under the Creative Commons Public Domain, for
// any use, without warranty, 2025 by Antmicro.
// SPDX-License-Identifier: CC0-1.0
//======================================================================

// For std::unique_ptr
#include <memory>

// SystemC global header
#include <systemc>

// Include common routines
#include <verilated.h>
#include <verilated_vcd_sc.h>

#include <sys/stat.h>  // mkdir

// Include model header, generated from Verilating "top.v"
#include "Vtop.h"

using namespace sc_core;
using namespace sc_dt;

static constexpr sc_time_unit sc_time_units = SC_NS;

struct ExtModuleSub final : public sc_module {
    sc_signal<bool> SC_NAMED(bit_nested);
    SC_CTOR(ExtModuleSub) {}
};

struct ExtModule final : public sc_module {
    // Custom signals to trace driven from SysC module
    sc_signal<bool> SC_NAMED(bit_signal, true);
    sc_signal<bool> SC_NAMED(bit_signal2, false);
    sc_signal<CData> SC_NAMED(int_signal, 12);
    sc_signal<CData> SC_NAMED(int_signal2, 123);
    sc_signal<SData> SC_NAMED(sdata_signal, std::numeric_limits<SData>::max());
    sc_signal<IData> SC_NAMED(idata_signal, std::numeric_limits<IData>::max());
    sc_signal<QData> SC_NAMED(qdata_signal, std::numeric_limits<QData>::max());
    sc_signal<double> SC_NAMED(double_signal, 1.0);
    sc_signal<sc_bv<3>> SC_NAMED(bv_signal, 2);
    sc_signal<sc_uint<3>> SC_NAMED(uint_signal, 3);
    sc_signal<sc_uint<62>> SC_NAMED(wide_uint_signal, std::pow(2, 62) - 1);
    sc_in<sc_uint<2>> SC_NAMED(uint_in);
    sc_out<sc_bv<512>> SC_NAMED(bv_out);

    ExtModuleSub SC_NAMED(sub);

    SC_CTOR(ExtModule) { SC_THREAD(tick); }

private:
    void tick() {
        while (!Verilated::gotFinish()) {
            // Change external signals
            bit_signal = !bit_signal;
            bit_signal2 = !bit_signal2;
            int_signal = int_signal + 1;
            int_signal2 = int_signal2 - 2;
            sdata_signal = sdata_signal - 1;
            idata_signal = idata_signal - 1;
            qdata_signal = qdata_signal - 1;
            double_signal = double_signal + 0.1;
            bv_signal = sc_bv<3>(bv_signal.read().to_uint() + 1);
            uint_signal = sc_uint<3>(uint_signal.read().to_uint() + 1);
            wide_uint_signal = sc_uint<62>(wide_uint_signal.read().to_uint64() - 1);

            std::printf(
                "[%f] bit_signal=%hhu int_signal=%hhu "
                "bit_signal2=%hhu int_signal2=%hhu sdata_signal=%u idata_signal=%hhu "
                "qdata_signal=%lu bv_signal=%u uint_signal=%u wide_uint_signal=%" PRIu64 "\n",
                sc_core::sc_time_stamp().to_double(), bit_signal.read(), int_signal.read(),
                bit_signal2.read(), int_signal2.read(), sdata_signal.read(), idata_signal.read(),
                qdata_signal.read(), bv_signal.read().to_uint(), uint_signal.read().to_uint(),
                static_cast<uint64_t>(wide_uint_signal.read().to_uint64()));
            wait(1, sc_time_units);
        }
    }
};

struct VtopWrapper : public sc_module {
    Vtop SC_NAMED(vtop);
    SC_CTOR(VtopWrapper){};
};
struct TopSysC final : public sc_module {
    ExtModule SC_NAMED(ext);
    VtopWrapper SC_NAMED(vtopWrapper);

    sc_signal<decltype(ext.uint_in)::data_type> SC_NAMED(uint_port, 123);
    sc_signal<decltype(ext.bv_out)::data_type> SC_NAMED(bv_port, 321);

    SC_CTOR(TopSysC) {
        ext.uint_in(uint_port);
        ext.bv_out(bv_port);
        SC_THREAD(tick);
    }

private:
    void tick() {
        while (!Verilated::gotFinish()) {
            using UintSigType = decltype(uint_port)::value_type;
            uint_port = UintSigType(uint_port.read().to_uint() + 1);
            using BvSigType = decltype(bv_port)::value_type;
            bv_port = BvSigType(bv_port.read().to_uint64() + 1);
            wait(1, sc_time_units);
        }
    }
};

int sc_main(int argc, char* argv[]) {
    // Set debug level, 0 is off, 9 is highest presently used
    // May be overridden by commandArgs argument parsing
    Verilated::debug(0);

    // Before any evaluation, need to know to calculate those signals only used for tracing
    Verilated::traceEverOn(true);

    // Pass arguments so Verilated code can see them, e.g. $value$plusargs
    // This needs to be called before you create any model
    Verilated::commandArgs(argc, argv);

    std::cout << "Enabling waves into logs/vlt_dump.vcd...\n";
    std::unique_ptr<VerilatedVcdSc> tfp = std::make_unique<VerilatedVcdSc>();
    TopSysC SC_NAMED(topSysC);

    // SystemC signal to trace connected to Verilog model
    sc_clock clk{"clk", 1, SC_NS, 0.5, 3, sc_time_units, true};
    topSysC.vtopWrapper.vtop.clk(clk);

    // Add desired signals to trace
    tfp->addTraceVar(topSysC.ext.bit_signal);
    tfp->addTraceVar(topSysC.ext.bit_signal2);
    tfp->addTraceVar(topSysC.ext.int_signal);
    tfp->addTraceVar(topSysC.ext.int_signal2);
    tfp->addTraceVar(topSysC.ext.sdata_signal);
    tfp->addTraceVar(topSysC.ext.idata_signal);
    tfp->addTraceVar(topSysC.ext.qdata_signal);
    tfp->addTraceVar(topSysC.ext.double_signal);
    tfp->addTraceVar(topSysC.ext.bv_signal);
    tfp->addTraceVar(topSysC.ext.uint_signal);
    tfp->addTraceVar(topSysC.ext.wide_uint_signal);

    // Add desired ports to trace
    tfp->addTraceVar(topSysC.ext.uint_in);
    tfp->addTraceVar(topSysC.ext.bv_out);

    // Hierarchy of traced signals is respected
    tfp->addTraceVar(topSysC.ext.sub.bit_nested);

    // You must do one evaluation before enabling waves, in order to allow
    // SystemC to interconnect everything for testing.
    sc_start(SC_ZERO_TIME);

    // Trace 99 levels of hierarchy
    topSysC.vtopWrapper.vtop.trace(tfp.get(), 99);

    Verilated::mkdir("logs");
    tfp->open("logs/vlt_dump.vcd");

    // Simulate until $finish
    while (!Verilated::gotFinish()) { sc_start(1, sc_time_units); }

    // Final model cleanup
    topSysC.vtopWrapper.vtop.final();

    return 0;
}
