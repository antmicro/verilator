// Verilated -*- C++ -*-
// DESCRIPTION: Verilator output: Design implementation internals
// See Vtop.h for the primary calling header

#include "Vtop__pch.h"

#ifdef VL_DEBUG
VL_ATTR_COLD void Vtop___024root___dump_triggers__ico(const VlUnpacked<QData/*63:0*/, 1> &triggers, const std::string &tag);
#endif  // VL_DEBUG

void Vtop___024root___eval_triggers__ico(Vtop___024root* vlSelf) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vtop___024root___eval_triggers__ico\n"); );
    Vtop__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    auto& vlSelfRef = std::ref(*vlSelf).get();
    // Body
    vlSelfRef.__VicoTriggered[0U] = ((0xfffffffffffffffeULL 
                                      & vlSelfRef.__VicoTriggered
                                      [0U]) | (IData)((IData)(vlSelfRef.__VicoFirstIteration)));
    vlSelfRef.__VicoFirstIteration = 0U;
#ifdef VL_DEBUG
    if (VL_UNLIKELY(vlSymsp->_vm_contextp__->debug())) {
        Vtop___024root___dump_triggers__ico(vlSelfRef.__VicoTriggered, "ico"s);
    }
#endif
}

bool Vtop___024root___trigger_anySet__ico(const VlUnpacked<QData/*63:0*/, 1> &in) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vtop___024root___trigger_anySet__ico\n"); );
    // Locals
    IData/*31:0*/ n;
    // Body
    n = 0U;
    do {
        if (in[n]) {
            return (1U);
        }
        n = ((IData)(1U) + n);
    } while ((1U > n));
    return (0U);
}

void Vtop___024root___ico_sequent__TOP__0(Vtop___024root* vlSelf) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vtop___024root___ico_sequent__TOP__0\n"); );
    Vtop__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    auto& vlSelfRef = std::ref(*vlSelf).get();
    // Locals
    VlWide<3>/*95:0*/ __Vtemp_1;
    VlWide<3>/*95:0*/ __Vtemp_3;
    VlWide<3>/*95:0*/ __Vtemp_4;
    VlWide<3>/*95:0*/ __Vtemp_5;
    VlWide<3>/*95:0*/ __Vtemp_6;
    VlWide<3>/*95:0*/ __Vtemp_7;
    // Body
    VL_ASSIGN_ISI(1, vlSelfRef.__Vcellinp__top__fastclk, vlSelfRef.fastclk);
    VL_ASSIGN_ISI(1, vlSelfRef.__Vcellinp__top__clk, vlSelfRef.clk);
    VL_ASSIGN_WSW(70, vlSelfRef.__Vcellinp__top__in_wide, vlSelfRef.in_wide);
    VL_ASSIGN_QSQ(40, vlSelfRef.__Vcellinp__top__in_quad, vlSelfRef.in_quad);
    VL_ASSIGN_ISI(2, vlSelfRef.__Vcellinp__top__in_small, vlSelfRef.in_small);
    VL_ASSIGN_ISI(1, vlSelfRef.__Vcellinp__top__reset_l, vlSelfRef.reset_l);
    if (((IData)(vlSelfRef.__Vcellinp__top__fastclk) 
         ^ (IData)(vlSelfRef.top__DOT____Vtogcov__fastclk))) {
        VL_COV_TOGGLE_CHG_ST_I(1, vlSymsp->__Vcoverage + 2, vlSelfRef.__Vcellinp__top__fastclk, vlSelfRef.top__DOT____Vtogcov__fastclk);
        vlSelfRef.top__DOT____Vtogcov__fastclk = vlSelfRef.__Vcellinp__top__fastclk;
    }
    if (((IData)(vlSelfRef.__Vcellinp__top__clk) ^ (IData)(vlSelfRef.top__DOT____Vtogcov__clk))) {
        VL_COV_TOGGLE_CHG_ST_I(1, vlSymsp->__Vcoverage + 0, vlSelfRef.__Vcellinp__top__clk, vlSelfRef.top__DOT____Vtogcov__clk);
        vlSelfRef.top__DOT____Vtogcov__clk = vlSelfRef.__Vcellinp__top__clk;
    }
    __Vtemp_1[0U] = (vlSelfRef.__Vcellinp__top__in_wide[0U] 
                     ^ vlSelfRef.top__DOT____Vtogcov__in_wide[0U]);
    __Vtemp_1[1U] = (vlSelfRef.__Vcellinp__top__in_wide[1U] 
                     ^ vlSelfRef.top__DOT____Vtogcov__in_wide[1U]);
    __Vtemp_1[2U] = (vlSelfRef.__Vcellinp__top__in_wide[2U] 
                     ^ vlSelfRef.top__DOT____Vtogcov__in_wide[2U]);
    if (__Vtemp_1) {
        VL_COV_TOGGLE_CHG_ST_W(70, vlSymsp->__Vcoverage + 314, vlSelfRef.__Vcellinp__top__in_wide, vlSelfRef.top__DOT____Vtogcov__in_wide);
        vlSelfRef.top__DOT____Vtogcov__in_wide[0U] 
            = vlSelfRef.__Vcellinp__top__in_wide[0U];
        vlSelfRef.top__DOT____Vtogcov__in_wide[1U] 
            = vlSelfRef.__Vcellinp__top__in_wide[1U];
        vlSelfRef.top__DOT____Vtogcov__in_wide[2U] 
            = vlSelfRef.__Vcellinp__top__in_wide[2U];
    }
    if ((vlSelfRef.__Vcellinp__top__in_quad ^ vlSelfRef.top__DOT____Vtogcov__in_quad)) {
        VL_COV_TOGGLE_CHG_ST_Q(40, vlSymsp->__Vcoverage + 234, vlSelfRef.__Vcellinp__top__in_quad, vlSelfRef.top__DOT____Vtogcov__in_quad);
        vlSelfRef.top__DOT____Vtogcov__in_quad = vlSelfRef.__Vcellinp__top__in_quad;
    }
    if (((IData)(vlSelfRef.__Vcellinp__top__in_small) 
         ^ (IData)(vlSelfRef.top__DOT____Vtogcov__in_small))) {
        VL_COV_TOGGLE_CHG_ST_I(2, vlSymsp->__Vcoverage + 230, vlSelfRef.__Vcellinp__top__in_small, vlSelfRef.top__DOT____Vtogcov__in_small);
        vlSelfRef.top__DOT____Vtogcov__in_small = vlSelfRef.__Vcellinp__top__in_small;
    }
    if (((IData)(vlSelfRef.__Vcellinp__top__reset_l) 
         ^ (IData)(vlSelfRef.top__DOT____Vtogcov__reset_l))) {
        VL_COV_TOGGLE_CHG_ST_I(1, vlSymsp->__Vcoverage + 4, vlSelfRef.__Vcellinp__top__reset_l, vlSelfRef.top__DOT____Vtogcov__reset_l);
        vlSelfRef.top__DOT____Vtogcov__reset_l = vlSelfRef.__Vcellinp__top__reset_l;
    }
    vlSelfRef.__Vcellout__top__out_small = (3U & ((IData)(vlSelfRef.__Vcellinp__top__reset_l)
                                                   ? 
                                                  ([&]() {
                    ++(vlSymsp->__Vcoverage[455]);
                }(), ((IData)(1U) + (IData)(vlSelfRef.__Vcellinp__top__in_small)))
                                                   : 
                                                  ([&]() {
                    ++(vlSymsp->__Vcoverage[454]);
                }(), 0U)));
    vlSelfRef.__Vcellout__top__out_quad = (0x000000ffffffffffULL 
                                           & ((IData)(vlSelfRef.__Vcellinp__top__reset_l)
                                               ? ([&]() {
                    ++(vlSymsp->__Vcoverage[457]);
                }(), (1ULL + vlSelfRef.__Vcellinp__top__in_quad))
                                               : ([&]() {
                    ++(vlSymsp->__Vcoverage[456]);
                }(), 0ULL)));
    __Vtemp_3[0U] = 1U;
    __Vtemp_3[1U] = 0U;
    __Vtemp_3[2U] = 0U;
    VL_ADD_W(3, __Vtemp_4, __Vtemp_3, vlSelfRef.__Vcellinp__top__in_wide);
    __Vtemp_5[0U] = 0U;
    __Vtemp_5[1U] = 0U;
    __Vtemp_5[2U] = 0U;
    VL_COND_WIWW(70, __Vtemp_6, (IData)(vlSelfRef.__Vcellinp__top__reset_l), 
                 ([&]() {
                ++(vlSymsp->__Vcoverage[459]);
            }(), __Vtemp_4), ([&]() {
                ++(vlSymsp->__Vcoverage[458]);
            }(), __Vtemp_5));
    vlSelfRef.__Vcellout__top__out_wide[0U] = __Vtemp_6[0U];
    vlSelfRef.__Vcellout__top__out_wide[1U] = __Vtemp_6[1U];
    vlSelfRef.__Vcellout__top__out_wide[2U] = (0x0000003fU 
                                               & __Vtemp_6[2U]);
    VL_ASSIGN_SII(2, vlSelfRef.out_small, vlSelfRef.__Vcellout__top__out_small);
    if (((IData)(vlSelfRef.__Vcellout__top__out_small) 
         ^ (IData)(vlSelfRef.top__DOT____Vtogcov__out_small))) {
        VL_COV_TOGGLE_CHG_ST_I(2, vlSymsp->__Vcoverage + 6, vlSelfRef.__Vcellout__top__out_small, vlSelfRef.top__DOT____Vtogcov__out_small);
        vlSelfRef.top__DOT____Vtogcov__out_small = vlSelfRef.__Vcellout__top__out_small;
    }
    VL_ASSIGN_SQQ(40, vlSelfRef.out_quad, vlSelfRef.__Vcellout__top__out_quad);
    if ((vlSelfRef.__Vcellout__top__out_quad ^ vlSelfRef.top__DOT____Vtogcov__out_quad)) {
        VL_COV_TOGGLE_CHG_ST_Q(40, vlSymsp->__Vcoverage + 10, vlSelfRef.__Vcellout__top__out_quad, vlSelfRef.top__DOT____Vtogcov__out_quad);
        vlSelfRef.top__DOT____Vtogcov__out_quad = vlSelfRef.__Vcellout__top__out_quad;
    }
    VL_ASSIGN_SWW(70, vlSelfRef.out_wide, vlSelfRef.__Vcellout__top__out_wide);
    __Vtemp_7[0U] = (vlSelfRef.__Vcellout__top__out_wide[0U] 
                     ^ vlSelfRef.top__DOT____Vtogcov__out_wide[0U]);
    __Vtemp_7[1U] = (vlSelfRef.__Vcellout__top__out_wide[1U] 
                     ^ vlSelfRef.top__DOT____Vtogcov__out_wide[1U]);
    __Vtemp_7[2U] = (vlSelfRef.__Vcellout__top__out_wide[2U] 
                     ^ vlSelfRef.top__DOT____Vtogcov__out_wide[2U]);
    if (__Vtemp_7) {
        VL_COV_TOGGLE_CHG_ST_W(70, vlSymsp->__Vcoverage + 90, vlSelfRef.__Vcellout__top__out_wide, vlSelfRef.top__DOT____Vtogcov__out_wide);
        vlSelfRef.top__DOT____Vtogcov__out_wide[0U] 
            = vlSelfRef.__Vcellout__top__out_wide[0U];
        vlSelfRef.top__DOT____Vtogcov__out_wide[1U] 
            = vlSelfRef.__Vcellout__top__out_wide[1U];
        vlSelfRef.top__DOT____Vtogcov__out_wide[2U] 
            = vlSelfRef.__Vcellout__top__out_wide[2U];
    }
}

void Vtop___024root___eval_ico(Vtop___024root* vlSelf) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vtop___024root___eval_ico\n"); );
    Vtop__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    auto& vlSelfRef = std::ref(*vlSelf).get();
    // Body
    if ((1ULL & vlSelfRef.__VicoTriggered[0U])) {
        Vtop___024root___ico_sequent__TOP__0(vlSelf);
        vlSelfRef.__Vm_traceActivity[1U] = 1U;
    }
}

bool Vtop___024root___eval_phase__ico(Vtop___024root* vlSelf) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vtop___024root___eval_phase__ico\n"); );
    Vtop__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    auto& vlSelfRef = std::ref(*vlSelf).get();
    // Locals
    CData/*0:0*/ __VicoExecute;
    // Body
    Vtop___024root___eval_triggers__ico(vlSelf);
    __VicoExecute = Vtop___024root___trigger_anySet__ico(vlSelfRef.__VicoTriggered);
    if (__VicoExecute) {
        Vtop___024root___eval_ico(vlSelf);
    }
    return (__VicoExecute);
}

#ifdef VL_DEBUG
VL_ATTR_COLD void Vtop___024root___dump_triggers__act(const VlUnpacked<QData/*63:0*/, 1> &triggers, const std::string &tag);
#endif  // VL_DEBUG

void Vtop___024root___eval_triggers__act(Vtop___024root* vlSelf) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vtop___024root___eval_triggers__act\n"); );
    Vtop__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    auto& vlSelfRef = std::ref(*vlSelf).get();
    // Body
    vlSelfRef.__VactTriggered[0U] = (QData)((IData)(
                                                    ((((IData)(vlSelfRef.__Vcellinp__top__clk) 
                                                       & (~ (IData)(vlSelfRef.__Vtrigprevexpr___TOP____Vcellinp__top__clk__0))) 
                                                      << 1U) 
                                                     | ((IData)(vlSelfRef.__Vcellinp__top__fastclk) 
                                                        & (~ (IData)(vlSelfRef.__Vtrigprevexpr___TOP____Vcellinp__top__fastclk__0))))));
    vlSelfRef.__Vtrigprevexpr___TOP____Vcellinp__top__fastclk__0 
        = vlSelfRef.__Vcellinp__top__fastclk;
    vlSelfRef.__Vtrigprevexpr___TOP____Vcellinp__top__clk__0 
        = vlSelfRef.__Vcellinp__top__clk;
#ifdef VL_DEBUG
    if (VL_UNLIKELY(vlSymsp->_vm_contextp__->debug())) {
        Vtop___024root___dump_triggers__act(vlSelfRef.__VactTriggered, "act"s);
    }
#endif
}

bool Vtop___024root___trigger_anySet__act(const VlUnpacked<QData/*63:0*/, 1> &in) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vtop___024root___trigger_anySet__act\n"); );
    // Locals
    IData/*31:0*/ n;
    // Body
    n = 0U;
    do {
        if (in[n]) {
            return (1U);
        }
        n = ((IData)(1U) + n);
    } while ((1U > n));
    return (0U);
}

void Vtop___024root___nba_sequent__TOP__0(Vtop___024root* vlSelf) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vtop___024root___nba_sequent__TOP__0\n"); );
    Vtop__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    auto& vlSelfRef = std::ref(*vlSelf).get();
    // Locals
    IData/*31:0*/ __Vdly__top__DOT__sub__DOT__count_c;
    __Vdly__top__DOT__sub__DOT__count_c = 0;
    // Body
    if (vlSymsp->_vm_contextp__->assertOnGet(1, 2)) {
        if (VL_LIKELY(((3U == vlSelfRef.top__DOT__sub__DOT__count_c)))) {
            ++(vlSymsp->__Vcoverage[605]);
        }
    }
    if (vlSymsp->_vm_contextp__->assertOnGet(2, 1)) {
        if (VL_UNLIKELY(((1U & (~ ((~ (IData)(vlSelfRef.__Vcellinp__top__reset_l)) 
                                   | (0x00000064U > vlSelfRef.top__DOT__sub__DOT__count_c))))))) {
            VL_WRITEF_NX("[%0t] %%Error: sub.v:52: Assertion failed in %Ntop.sub.AssertionExample: 'assert' failed.\n",0,
                         64,VL_TIME_UNITED_Q(1),-12,
                         vlSymsp->name());
            VL_STOP_MT("sub.v", 52, "");
        }
    }
    if ((0x00000064U > vlSelfRef.top__DOT__sub__DOT__count_c)) {
        ++(vlSymsp->__Vcoverage[601]);
    }
    if ((1U & (~ (IData)(vlSelfRef.__Vcellinp__top__reset_l)))) {
        ++(vlSymsp->__Vcoverage[602]);
    }
    if (((IData)(vlSelfRef.__Vcellinp__top__reset_l) 
         & (0x00000064U <= vlSelfRef.top__DOT__sub__DOT__count_c))) {
        ++(vlSymsp->__Vcoverage[603]);
    }
    ++(vlSymsp->__Vcoverage[604]);
    __Vdly__top__DOT__sub__DOT__count_c = vlSelfRef.top__DOT__sub__DOT__count_c;
    if (vlSelfRef.__Vcellinp__top__reset_l) {
        __Vdly__top__DOT__sub__DOT__count_c = ((IData)(1U) 
                                               + vlSelfRef.top__DOT__sub__DOT__count_c);
        if (VL_UNLIKELY(((3U <= vlSelfRef.top__DOT__sub__DOT__count_c)))) {
            VL_WRITEF_NX("[%0t] fastclk is %0# times faster than clk\n\n*-* All Finished *-*\n",0,
                         64,VL_TIME_UNITED_Q(1),-12,
                         32,VL_DIV_III(32, vlSelfRef.top__DOT__sub__DOT__count_f, vlSelfRef.top__DOT__sub__DOT__count_c));
            VL_FINISH_MT("sub.v", 45, "");
            ++(vlSymsp->__Vcoverage[594]);
        } else {
            ++(vlSymsp->__Vcoverage[595]);
        }
        ++(vlSymsp->__Vcoverage[597]);
    } else {
        ++(vlSymsp->__Vcoverage[596]);
        __Vdly__top__DOT__sub__DOT__count_c = 0U;
    }
    if ((1U & (~ (IData)(vlSelfRef.__Vcellinp__top__reset_l)))) {
        ++(vlSymsp->__Vcoverage[598]);
    }
    if (vlSelfRef.__Vcellinp__top__reset_l) {
        ++(vlSymsp->__Vcoverage[599]);
    }
    ++(vlSymsp->__Vcoverage[600]);
    vlSelfRef.top__DOT__sub__DOT__count_c = __Vdly__top__DOT__sub__DOT__count_c;
    if ((vlSelfRef.top__DOT__sub__DOT__count_c ^ vlSelfRef.top__DOT__sub__DOT____Vtogcov__count_c)) {
        VL_COV_TOGGLE_CHG_ST_I(32, vlSymsp->__Vcoverage + 530, vlSelfRef.top__DOT__sub__DOT__count_c, vlSelfRef.top__DOT__sub__DOT____Vtogcov__count_c);
        vlSelfRef.top__DOT__sub__DOT____Vtogcov__count_c 
            = vlSelfRef.top__DOT__sub__DOT__count_c;
    }
}

void Vtop___024root___nba_sequent__TOP__1(Vtop___024root* vlSelf) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vtop___024root___nba_sequent__TOP__1\n"); );
    Vtop__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    auto& vlSelfRef = std::ref(*vlSelf).get();
    // Body
    if (vlSelfRef.__Vcellinp__top__reset_l) {
        vlSelfRef.top__DOT__sub__DOT__count_f = ((IData)(1U) 
                                                 + vlSelfRef.top__DOT__sub__DOT__count_f);
        ++(vlSymsp->__Vcoverage[526]);
    } else {
        ++(vlSymsp->__Vcoverage[525]);
        vlSelfRef.top__DOT__sub__DOT__count_f = 0U;
    }
    if ((1U & (~ (IData)(vlSelfRef.__Vcellinp__top__reset_l)))) {
        ++(vlSymsp->__Vcoverage[527]);
    }
    if (vlSelfRef.__Vcellinp__top__reset_l) {
        ++(vlSymsp->__Vcoverage[528]);
    }
    ++(vlSymsp->__Vcoverage[529]);
    if ((vlSelfRef.top__DOT__sub__DOT__count_f ^ vlSelfRef.top__DOT__sub__DOT____Vtogcov__count_f)) {
        VL_COV_TOGGLE_CHG_ST_I(32, vlSymsp->__Vcoverage + 461, vlSelfRef.top__DOT__sub__DOT__count_f, vlSelfRef.top__DOT__sub__DOT____Vtogcov__count_f);
        vlSelfRef.top__DOT__sub__DOT____Vtogcov__count_f 
            = vlSelfRef.top__DOT__sub__DOT__count_f;
    }
}

void Vtop___024root___eval_nba(Vtop___024root* vlSelf) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vtop___024root___eval_nba\n"); );
    Vtop__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    auto& vlSelfRef = std::ref(*vlSelf).get();
    // Body
    if ((2ULL & vlSelfRef.__VnbaTriggered[0U])) {
        Vtop___024root___nba_sequent__TOP__0(vlSelf);
    }
    if ((1ULL & vlSelfRef.__VnbaTriggered[0U])) {
        Vtop___024root___nba_sequent__TOP__1(vlSelf);
    }
}

void Vtop___024root___trigger_orInto__act(VlUnpacked<QData/*63:0*/, 1> &out, const VlUnpacked<QData/*63:0*/, 1> &in) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vtop___024root___trigger_orInto__act\n"); );
    // Locals
    IData/*31:0*/ n;
    // Body
    n = 0U;
    do {
        out[n] = (out[n] | in[n]);
        n = ((IData)(1U) + n);
    } while ((1U > n));
}

bool Vtop___024root___eval_phase__act(Vtop___024root* vlSelf) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vtop___024root___eval_phase__act\n"); );
    Vtop__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    auto& vlSelfRef = std::ref(*vlSelf).get();
    // Body
    Vtop___024root___eval_triggers__act(vlSelf);
    Vtop___024root___trigger_orInto__act(vlSelfRef.__VnbaTriggered, vlSelfRef.__VactTriggered);
    return (0U);
}

void Vtop___024root___trigger_clear__act(VlUnpacked<QData/*63:0*/, 1> &out) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vtop___024root___trigger_clear__act\n"); );
    // Locals
    IData/*31:0*/ n;
    // Body
    n = 0U;
    do {
        out[n] = 0ULL;
        n = ((IData)(1U) + n);
    } while ((1U > n));
}

bool Vtop___024root___eval_phase__nba(Vtop___024root* vlSelf) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vtop___024root___eval_phase__nba\n"); );
    Vtop__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    auto& vlSelfRef = std::ref(*vlSelf).get();
    // Locals
    CData/*0:0*/ __VnbaExecute;
    // Body
    __VnbaExecute = Vtop___024root___trigger_anySet__act(vlSelfRef.__VnbaTriggered);
    if (__VnbaExecute) {
        Vtop___024root___eval_nba(vlSelf);
        Vtop___024root___trigger_clear__act(vlSelfRef.__VnbaTriggered);
    }
    return (__VnbaExecute);
}

void Vtop___024root___eval(Vtop___024root* vlSelf) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vtop___024root___eval\n"); );
    Vtop__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    auto& vlSelfRef = std::ref(*vlSelf).get();
    // Locals
    IData/*31:0*/ __VicoIterCount;
    IData/*31:0*/ __VnbaIterCount;
    // Body
    __VicoIterCount = 0U;
    vlSelfRef.__VicoFirstIteration = 1U;
    do {
        if (VL_UNLIKELY(((0x00000064U < __VicoIterCount)))) {
#ifdef VL_DEBUG
            Vtop___024root___dump_triggers__ico(vlSelfRef.__VicoTriggered, "ico"s);
#endif
            VL_FATAL_MT("top.v", 11, "", "Input combinational region did not converge after 100 tries");
        }
        __VicoIterCount = ((IData)(1U) + __VicoIterCount);
    } while (Vtop___024root___eval_phase__ico(vlSelf));
    __VnbaIterCount = 0U;
    do {
        if (VL_UNLIKELY(((0x00000064U < __VnbaIterCount)))) {
#ifdef VL_DEBUG
            Vtop___024root___dump_triggers__act(vlSelfRef.__VnbaTriggered, "nba"s);
#endif
            VL_FATAL_MT("top.v", 11, "", "NBA region did not converge after 100 tries");
        }
        __VnbaIterCount = ((IData)(1U) + __VnbaIterCount);
        vlSelfRef.__VactIterCount = 0U;
        do {
            if (VL_UNLIKELY(((0x00000064U < vlSelfRef.__VactIterCount)))) {
#ifdef VL_DEBUG
                Vtop___024root___dump_triggers__act(vlSelfRef.__VactTriggered, "act"s);
#endif
                VL_FATAL_MT("top.v", 11, "", "Active region did not converge after 100 tries");
            }
            vlSelfRef.__VactIterCount = ((IData)(1U) 
                                         + vlSelfRef.__VactIterCount);
        } while (Vtop___024root___eval_phase__act(vlSelf));
    } while (Vtop___024root___eval_phase__nba(vlSelf));
}

#ifdef VL_DEBUG
void Vtop___024root___eval_debug_assertions(Vtop___024root* vlSelf) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vtop___024root___eval_debug_assertions\n"); );
    Vtop__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    auto& vlSelfRef = std::ref(*vlSelf).get();
}
#endif  // VL_DEBUG
