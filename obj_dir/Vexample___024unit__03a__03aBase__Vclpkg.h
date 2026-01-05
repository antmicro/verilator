// Verilated -*- C++ -*-
// DESCRIPTION: Verilator output: Design internal header
// See Vexample.h for the primary calling header

#ifndef VERILATED_VEXAMPLE___024UNIT__03A__03ABASE__VCLPKG_H_
#define VERILATED_VEXAMPLE___024UNIT__03A__03ABASE__VCLPKG_H_  // guard

#include "verilated.h"
#include "verilated_timing.h"
class Vexample___024unit__03a__03aBase;
class Vexample_std__03a__03aprocess;


class Vexample__Syms;

class alignas(VL_CACHE_LINE_BYTES) Vexample___024unit__03a__03aBase__Vclpkg final {
  public:

    // INTERNAL VARIABLES
    Vexample__Syms* vlSymsp;
    const char* vlNamep;

    // CONSTRUCTORS
    Vexample___024unit__03a__03aBase__Vclpkg() = default;
    ~Vexample___024unit__03a__03aBase__Vclpkg() = default;
    void ctor(Vexample__Syms* symsp, const char* namep);
    void dtor();
    VL_UNCOPYABLE(Vexample___024unit__03a__03aBase__Vclpkg);

    // INTERNAL METHODS
    void __Vconfigure(bool first);
};


class Vexample__Syms;

class Vexample___024unit__03a__03aBase : public virtual VlClass {
  public:

    // DESIGN SPECIFIC STATE
    CData/*0:0*/ __Vtrigprevexpr_hcce7cd61__0;
    IData/*31:0*/ __PVT__k;
    void __VnoInFunc_get_val(Vexample__Syms* __restrict vlSymsp, std::string &get_val__Vfuncrtn);
  private:
    void _ctor_var_reset(Vexample__Syms* __restrict vlSymsp);
  public:
    Vexample___024unit__03a__03aBase(VlProcessRef vlProcess, Vexample__Syms* __restrict vlSymsp, std::string foo);
  private:
    VlCoroutine new____Vfork_1__0(VlProcessRef vlProcess, Vexample__Syms* __restrict vlSymsp, VlClassRef<Vexample_std__03a__03aprocess> unnamedblk1__DOT____VforkParent);
  public:
    std::string to_string() const;
    std::string to_string_middle() const;
    virtual ~Vexample___024unit__03a__03aBase();
};

std::string VL_TO_STRING(const VlClassRef<Vexample___024unit__03a__03aBase>& obj);

#endif  // guard
