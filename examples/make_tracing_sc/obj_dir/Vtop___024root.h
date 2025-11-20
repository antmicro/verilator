// Verilated -*- C++ -*-
// DESCRIPTION: Verilator output: Design internal header
// See Vtop.h for the primary calling header

#ifndef VERILATED_VTOP___024ROOT_H_
#define VERILATED_VTOP___024ROOT_H_  // guard

#include "verilated.h"
#include "verilated_sc.h"
#include "verilated_cov.h"


class Vtop__Syms;

class alignas(VL_CACHE_LINE_BYTES) Vtop___024root final {
  public:

    // DESIGN SPECIFIC STATE
    CData/*1:0*/ __Vcellinp__top__in_small;
    CData/*1:0*/ __Vcellout__top__out_small;
    CData/*0:0*/ __Vcellinp__top__reset_l;
    CData/*0:0*/ __Vcellinp__top__fastclk;
    CData/*0:0*/ __Vcellinp__top__clk;
    CData/*0:0*/ top__DOT____Vtogcov__clk;
    CData/*0:0*/ top__DOT____Vtogcov__fastclk;
    CData/*0:0*/ top__DOT____Vtogcov__reset_l;
    CData/*1:0*/ top__DOT____Vtogcov__out_small;
    CData/*1:0*/ top__DOT____Vtogcov__in_small;
    CData/*0:0*/ __VstlFirstIteration;
    CData/*0:0*/ __VicoFirstIteration;
    CData/*0:0*/ __Vtrigprevexpr___TOP____Vcellinp__top__fastclk__0;
    CData/*0:0*/ __Vtrigprevexpr___TOP____Vcellinp__top__clk__0;
    VlWide<3>/*69:0*/ __Vcellinp__top__in_wide;
    VlWide<3>/*69:0*/ __Vcellout__top__out_wide;
    VlWide<3>/*69:0*/ top__DOT____Vtogcov__out_wide;
    VlWide<3>/*69:0*/ top__DOT____Vtogcov__in_wide;
    IData/*31:0*/ top__DOT__sub__DOT__count_f;
    IData/*31:0*/ top__DOT__sub__DOT__count_c;
    IData/*31:0*/ top__DOT__sub__DOT____Vtogcov__count_f;
    IData/*31:0*/ top__DOT__sub__DOT____Vtogcov__count_c;
    IData/*31:0*/ __VactIterCount;
    QData/*39:0*/ __Vcellinp__top__in_quad;
    QData/*39:0*/ __Vcellout__top__out_quad;
    QData/*39:0*/ top__DOT____Vtogcov__out_quad;
    QData/*39:0*/ top__DOT____Vtogcov__in_quad;
    VlUnpacked<QData/*63:0*/, 1> __VstlTriggered;
    VlUnpacked<QData/*63:0*/, 1> __VicoTriggered;
    VlUnpacked<QData/*63:0*/, 1> __VactTriggered;
    VlUnpacked<QData/*63:0*/, 1> __VnbaTriggered;
    VlUnpacked<CData/*0:0*/, 2> __Vm_traceActivity;
    sc_core::sc_in<bool> clk;
    sc_core::sc_in<bool> fastclk;
    sc_core::sc_in<bool> reset_l;
    sc_core::sc_out<uint32_t> out_small;
    sc_core::sc_in<uint32_t> in_small;
    sc_core::sc_out<uint64_t> out_quad;
    sc_core::sc_in<uint64_t> in_quad;
    sc_core::sc_out<sc_dt::sc_bv<70> > out_wide;
    sc_core::sc_in<sc_dt::sc_bv<70> > in_wide;

    // INTERNAL VARIABLES
    Vtop__Syms* vlSymsp;
    const char* vlNamep;

    // CONSTRUCTORS
    Vtop___024root(Vtop__Syms* symsp, const char* namep);
    ~Vtop___024root();
    VL_UNCOPYABLE(Vtop___024root);

    // INTERNAL METHODS
    void __Vconfigure(bool first);
    void __vlCoverInsert(uint32_t* countp, bool enable, const char* filenamep, int lineno, int column,
        const char* hierp, const char* pagep, const char* commentp, const char* linescovp);
    void __vlCoverToggleInsert(int begin, int end, bool ranged, uint32_t* countp, bool enable, const char* filenamep, int lineno, int column,
        const char* hierp, const char* pagep, const char* commentp);
};


#endif  // guard
