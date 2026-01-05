// Verilated -*- C++ -*-
// DESCRIPTION: Verilator output: Design internal header
// See Vexample.h for the primary calling header

#ifndef VERILATED_VEXAMPLE___024UNIT__03A__03ADERIVED__VCLPKG_H_
#define VERILATED_VEXAMPLE___024UNIT__03A__03ADERIVED__VCLPKG_H_  // guard

#include "verilated.h"
#include "verilated_timing.h"
class Vexample___024unit__03a__03aBase;


class Vexample__Syms;

class alignas(VL_CACHE_LINE_BYTES) Vexample___024unit__03a__03aDerived__Vclpkg final {
  public:

    // INTERNAL VARIABLES
    Vexample__Syms* vlSymsp;
    const char* vlNamep;

    // CONSTRUCTORS
    Vexample___024unit__03a__03aDerived__Vclpkg() = default;
    ~Vexample___024unit__03a__03aDerived__Vclpkg() = default;
    void ctor(Vexample__Syms* symsp, const char* namep);
    void dtor();
    VL_UNCOPYABLE(Vexample___024unit__03a__03aDerived__Vclpkg);

    // INTERNAL METHODS
    void __Vconfigure(bool first);
};

#include "Vexample___024unit__03a__03aBase__Vclpkg.h"

class Vexample__Syms;

class Vexample___024unit__03a__03aDerived : public Vexample___024unit__03a__03aBase {
  public:

    // DESIGN SPECIFIC STATE
    std::string __Vfunc_get_val__1__Vfuncout;
  private:
    void _ctor_var_reset(Vexample__Syms* __restrict vlSymsp);
  public:
    Vexample___024unit__03a__03aDerived(VlProcessRef vlProcess, Vexample__Syms* __restrict vlSymsp);
    std::string to_string() const;
    std::string to_string_middle() const;
    virtual ~Vexample___024unit__03a__03aDerived();
};

std::string VL_TO_STRING(const VlClassRef<Vexample___024unit__03a__03aDerived>& obj);

#endif  // guard
