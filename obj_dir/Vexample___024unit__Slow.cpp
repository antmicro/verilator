// Verilated -*- C++ -*-
// DESCRIPTION: Verilator output: Design implementation internals
// See Vexample.h for the primary calling header

#include "Vexample__pch.h"

void Vexample___024unit___ctor_var_reset(Vexample___024unit* vlSelf);

void Vexample___024unit::ctor(Vexample__Syms* symsp, const char* namep) {
    vlSymsp = symsp;
    vlNamep = strdup(Verilated::catName(vlSymsp->name(), namep));
    // Reset structure values
    Vexample___024unit___ctor_var_reset(this);
}

void Vexample___024unit::__Vconfigure(bool first) {
    (void)first;  // Prevent unused variable warning
}

void Vexample___024unit::dtor() {
    VL_DO_DANGLING(std::free(const_cast<char*>(vlNamep)), vlNamep);
}
