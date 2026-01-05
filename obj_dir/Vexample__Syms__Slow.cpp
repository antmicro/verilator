// Verilated -*- C++ -*-
// DESCRIPTION: Verilator output: Symbol table implementation internals

#include "Vexample__pch.h"

Vexample__Syms::Vexample__Syms(VerilatedContext* contextp, const char* namep, Vexample* modelp)
    : VerilatedSyms{contextp}
    // Setup internal state of the Syms class
    , __Vm_modelp{modelp}
    // Setup top module instance
    , TOP{this, namep}
{
    // Check resources
    Verilated::stackCheck(281);
    // Setup sub module instances
    TOP____024unit__03a__03aBase__Vclpkg.ctor(this, "$unit::Base__Vclpkg");
    TOP____024unit__03a__03aDerived__Vclpkg.ctor(this, "$unit::Derived__Vclpkg");
    TOP____024unit.ctor(this, "$unit");
    TOP__std.ctor(this, "std");
    TOP__std__03a__03aprocess__Vclpkg.ctor(this, "std::process__Vclpkg");
    TOP__std__03a__03asemaphore__Vclpkg.ctor(this, "std::semaphore__Vclpkg");
    // Configure time unit / time precision
    _vm_contextp__->timeunit(-12);
    _vm_contextp__->timeprecision(-12);
    // Setup each module's pointers to their submodules
    TOP.__024unit__03a__03aBase__Vclpkg = &TOP____024unit__03a__03aBase__Vclpkg;
    TOP.__024unit__03a__03aDerived__Vclpkg = &TOP____024unit__03a__03aDerived__Vclpkg;
    TOP.__PVT____024unit = &TOP____024unit;
    TOP.__PVT__std = &TOP__std;
    TOP.std__03a__03aprocess__Vclpkg = &TOP__std__03a__03aprocess__Vclpkg;
    TOP.std__03a__03asemaphore__Vclpkg = &TOP__std__03a__03asemaphore__Vclpkg;
    // Setup each module's pointer back to symbol table (for public functions)
    TOP.__Vconfigure(true);
    TOP____024unit__03a__03aBase__Vclpkg.__Vconfigure(true);
    TOP____024unit__03a__03aDerived__Vclpkg.__Vconfigure(true);
    TOP____024unit.__Vconfigure(true);
    TOP__std.__Vconfigure(true);
    TOP__std__03a__03aprocess__Vclpkg.__Vconfigure(true);
    TOP__std__03a__03asemaphore__Vclpkg.__Vconfigure(true);
    // Setup scopes
}

Vexample__Syms::~Vexample__Syms() {
    // Tear down scopes
    // Tear down sub module instances
    TOP__std__03a__03asemaphore__Vclpkg.dtor();
    TOP__std__03a__03aprocess__Vclpkg.dtor();
    TOP__std.dtor();
    TOP____024unit.dtor();
    TOP____024unit__03a__03aDerived__Vclpkg.dtor();
    TOP____024unit__03a__03aBase__Vclpkg.dtor();
}
