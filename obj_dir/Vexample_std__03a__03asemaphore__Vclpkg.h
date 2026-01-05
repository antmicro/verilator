// Verilated -*- C++ -*-
// DESCRIPTION: Verilator output: Design internal header
// See Vexample.h for the primary calling header

#ifndef VERILATED_VEXAMPLE_STD__03A__03ASEMAPHORE__VCLPKG_H_
#define VERILATED_VEXAMPLE_STD__03A__03ASEMAPHORE__VCLPKG_H_  // guard

#include "verilated.h"
#include "verilated_timing.h"


class Vexample__Syms;

class alignas(VL_CACHE_LINE_BYTES) Vexample_std__03a__03asemaphore__Vclpkg final {
  public:

    // INTERNAL VARIABLES
    Vexample__Syms* vlSymsp;
    const char* vlNamep;

    // CONSTRUCTORS
    Vexample_std__03a__03asemaphore__Vclpkg() = default;
    ~Vexample_std__03a__03asemaphore__Vclpkg() = default;
    void ctor(Vexample__Syms* symsp, const char* namep);
    void dtor();
    VL_UNCOPYABLE(Vexample_std__03a__03asemaphore__Vclpkg);

    // INTERNAL METHODS
    void __Vconfigure(bool first);
};


class Vexample__Syms;

class Vexample_std__03a__03asemaphore : public virtual VlClass {
  public:

    // DESIGN SPECIFIC STATE
    CData/*0:0*/ __Vtrigprevexpr_ha2e26130__0;
    IData/*31:0*/ __PVT__m_keyCount;
    VlCoroutine __VnoInFunc_get(Vexample__Syms* __restrict vlSymsp, IData/*31:0*/ keyCount);
    void __VnoInFunc_put(Vexample__Syms* __restrict vlSymsp, IData/*31:0*/ keyCount);
    void __VnoInFunc_try_get(Vexample__Syms* __restrict vlSymsp, IData/*31:0*/ keyCount, IData/*31:0*/ &try_get__Vfuncrtn);
  private:
    void _ctor_var_reset(Vexample__Syms* __restrict vlSymsp);
  public:
    Vexample_std__03a__03asemaphore(Vexample__Syms* __restrict vlSymsp, IData/*31:0*/ keyCount);
    std::string to_string() const;
    std::string to_string_middle() const;
    ~Vexample_std__03a__03asemaphore() {}
};

std::string VL_TO_STRING(const VlClassRef<Vexample_std__03a__03asemaphore>& obj);

#endif  // guard
