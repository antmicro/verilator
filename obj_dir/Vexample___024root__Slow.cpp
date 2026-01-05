// Verilated -*- C++ -*-
// DESCRIPTION: Verilator output: Design implementation internals
// See Vexample.h for the primary calling header

#include "Vexample__pch.h"

void Vexample___024root___ctor_var_reset(Vexample___024root* vlSelf);

Vexample___024root::Vexample___024root(Vexample__Syms* symsp, const char* namep)
    : __VdlySched{*symsp->_vm_contextp__}
 {
    vlSymsp = symsp;
    vlNamep = strdup(namep);
    // Reset structure values
    Vexample___024root___ctor_var_reset(this);
}

void Vexample___024root::__Vconfigure(bool first) {
    (void)first;  // Prevent unused variable warning
}

Vexample___024root::~Vexample___024root() {
    VL_DO_DANGLING(std::free(const_cast<char*>(vlNamep)), vlNamep);
}
