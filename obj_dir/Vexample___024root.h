// Verilated -*- C++ -*-
// DESCRIPTION: Verilator output: Design internal header
// See Vexample.h for the primary calling header

#ifndef VERILATED_VEXAMPLE___024ROOT_H_
#define VERILATED_VEXAMPLE___024ROOT_H_  // guard

#include "verilated.h"
#include "verilated_timing.h"
class Vexample___024unit;
class Vexample___024unit__03a__03aBase__Vclpkg;
class Vexample___024unit__03a__03aDerived;
class Vexample___024unit__03a__03aDerived__Vclpkg;
class Vexample_std;
class Vexample_std__03a__03aprocess__Vclpkg;
class Vexample_std__03a__03asemaphore__Vclpkg;


class Vexample__Syms;

class alignas(VL_CACHE_LINE_BYTES) Vexample___024root final {
  public:
    // CELLS
    Vexample___024unit* __PVT____024unit;
    Vexample_std* __PVT__std;
    Vexample___024unit__03a__03aBase__Vclpkg* __024unit__03a__03aBase__Vclpkg;
    Vexample___024unit__03a__03aDerived__Vclpkg* __024unit__03a__03aDerived__Vclpkg;
    Vexample_std__03a__03asemaphore__Vclpkg* std__03a__03asemaphore__Vclpkg;
    Vexample_std__03a__03aprocess__Vclpkg* std__03a__03aprocess__Vclpkg;

    // DESIGN SPECIFIC STATE
    IData/*31:0*/ __VactIterCount;
    VlUnpacked<QData/*63:0*/, 1> __VactTriggered;
    VlUnpacked<QData/*63:0*/, 1> __VnbaTriggered;
    VlDelayScheduler __VdlySched;
    VlDynamicTriggerScheduler __VdynSched;

    // INTERNAL VARIABLES
    Vexample__Syms* vlSymsp;
    const char* vlNamep;

    // CONSTRUCTORS
    Vexample___024root(Vexample__Syms* symsp, const char* namep);
    ~Vexample___024root();
    VL_UNCOPYABLE(Vexample___024root);

    // INTERNAL METHODS
    void __Vconfigure(bool first);
};


#endif  // guard
