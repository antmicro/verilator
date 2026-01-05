// Verilated -*- C++ -*-
// DESCRIPTION: Verilator output: Design internal header
// See Vexample.h for the primary calling header

#ifndef VERILATED_VEXAMPLE_STD_H_
#define VERILATED_VEXAMPLE_STD_H_  // guard

#include "verilated.h"
#include "verilated_timing.h"


class Vexample__Syms;

class alignas(VL_CACHE_LINE_BYTES) Vexample_std final {
  public:

    // INTERNAL VARIABLES
    Vexample__Syms* vlSymsp;
    const char* vlNamep;

    // CONSTRUCTORS
    Vexample_std() = default;
    ~Vexample_std() = default;
    void ctor(Vexample__Syms* symsp, const char* namep);
    void dtor();
    VL_UNCOPYABLE(Vexample_std);

    // INTERNAL METHODS
    void __Vconfigure(bool first);
};


#endif  // guard
