// Verilated -*- C++ -*-
// DESCRIPTION: Verilator output: Symbol table internal header
//
// Internal details; most calling programs do not need this header,
// unless using verilator public meta comments.

#ifndef VERILATED_VEXAMPLE__SYMS_H_
#define VERILATED_VEXAMPLE__SYMS_H_  // guard

#include "verilated.h"

// INCLUDE MODEL CLASS

#include "Vexample.h"

// INCLUDE MODULE CLASSES
#include "Vexample___024root.h"
#include "Vexample___024unit.h"
#include "Vexample_std.h"
#include "Vexample___024unit__03a__03aBase__Vclpkg.h"
#include "Vexample___024unit__03a__03aDerived__Vclpkg.h"
#include "Vexample_std__03a__03asemaphore__Vclpkg.h"
#include "Vexample_std__03a__03aprocess__Vclpkg.h"

// SYMS CLASS (contains all model state)
class alignas(VL_CACHE_LINE_BYTES) Vexample__Syms final : public VerilatedSyms {
  public:
    // INTERNAL STATE
    Vexample* const __Vm_modelp;
    std::vector<VlEvent*> __Vm_triggeredEvents;
    VlDeleter __Vm_deleter;
    bool __Vm_didInit = false;

    // MODULE INSTANCE STATE
    Vexample___024root             TOP;
    Vexample___024unit__03a__03aBase__Vclpkg TOP____024unit__03a__03aBase__Vclpkg;
    Vexample___024unit__03a__03aDerived__Vclpkg TOP____024unit__03a__03aDerived__Vclpkg;
    Vexample___024unit             TOP____024unit;
    Vexample_std                   TOP__std;
    Vexample_std__03a__03aprocess__Vclpkg TOP__std__03a__03aprocess__Vclpkg;
    Vexample_std__03a__03asemaphore__Vclpkg TOP__std__03a__03asemaphore__Vclpkg;

    // CONSTRUCTORS
    Vexample__Syms(VerilatedContext* contextp, const char* namep, Vexample* modelp);
    ~Vexample__Syms();

    // METHODS
    const char* name() const { return TOP.vlNamep; }
    void fireEvent(VlEvent& event) {
        if (VL_LIKELY(!event.isTriggered())) {
            __Vm_triggeredEvents.push_back(&event);
        }
        event.fire();
    }
    void clearTriggeredEvents() {
        for (const auto eventp : __Vm_triggeredEvents) eventp->clearTriggered();
        __Vm_triggeredEvents.clear();
    }
};

#endif  // guard
