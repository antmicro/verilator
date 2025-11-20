// Verilated -*- C++ -*-
// DESCRIPTION: Verilator output: Design implementation internals
// See Vtop.h for the primary calling header

#include "Vtop__pch.h"

VL_ATTR_COLD void Vtop___024root___eval_static(Vtop___024root* vlSelf) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vtop___024root___eval_static\n"); );
    Vtop__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    auto& vlSelfRef = std::ref(*vlSelf).get();
    // Body
    vlSelfRef.__Vtrigprevexpr___TOP____Vcellinp__top__fastclk__0 
        = vlSelfRef.__Vcellinp__top__fastclk;
    vlSelfRef.__Vtrigprevexpr___TOP____Vcellinp__top__clk__0 
        = vlSelfRef.__Vcellinp__top__clk;
}

VL_ATTR_COLD void Vtop___024root___eval_initial__TOP(Vtop___024root* vlSelf);

VL_ATTR_COLD void Vtop___024root___eval_initial(Vtop___024root* vlSelf) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vtop___024root___eval_initial\n"); );
    Vtop__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    auto& vlSelfRef = std::ref(*vlSelf).get();
    // Body
    Vtop___024root___eval_initial__TOP(vlSelf);
}

VL_ATTR_COLD void Vtop___024root___eval_initial__TOP(Vtop___024root* vlSelf) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vtop___024root___eval_initial__TOP\n"); );
    Vtop__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    auto& vlSelfRef = std::ref(*vlSelf).get();
    // Body
    VL_WRITEF_NX("[%0t] Model running...\n\n",0,64,
                 VL_TIME_UNITED_Q(1),-12);
    ++(vlSymsp->__Vcoverage[460]);
}

VL_ATTR_COLD void Vtop___024root___eval_final(Vtop___024root* vlSelf) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vtop___024root___eval_final\n"); );
    Vtop__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    auto& vlSelfRef = std::ref(*vlSelf).get();
}

#ifdef VL_DEBUG
VL_ATTR_COLD void Vtop___024root___dump_triggers__stl(const VlUnpacked<QData/*63:0*/, 1> &triggers, const std::string &tag);
#endif  // VL_DEBUG
VL_ATTR_COLD bool Vtop___024root___eval_phase__stl(Vtop___024root* vlSelf);

VL_ATTR_COLD void Vtop___024root___eval_settle(Vtop___024root* vlSelf) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vtop___024root___eval_settle\n"); );
    Vtop__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    auto& vlSelfRef = std::ref(*vlSelf).get();
    // Locals
    IData/*31:0*/ __VstlIterCount;
    // Body
    __VstlIterCount = 0U;
    vlSelfRef.__VstlFirstIteration = 1U;
    do {
        if (VL_UNLIKELY(((0x00000064U < __VstlIterCount)))) {
#ifdef VL_DEBUG
            Vtop___024root___dump_triggers__stl(vlSelfRef.__VstlTriggered, "stl"s);
#endif
            VL_FATAL_MT("top.v", 11, "", "Settle region did not converge after 100 tries");
        }
        __VstlIterCount = ((IData)(1U) + __VstlIterCount);
    } while (Vtop___024root___eval_phase__stl(vlSelf));
}

VL_ATTR_COLD void Vtop___024root___eval_triggers__stl(Vtop___024root* vlSelf) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vtop___024root___eval_triggers__stl\n"); );
    Vtop__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    auto& vlSelfRef = std::ref(*vlSelf).get();
    // Body
    vlSelfRef.__VstlTriggered[0U] = ((0xfffffffffffffffeULL 
                                      & vlSelfRef.__VstlTriggered
                                      [0U]) | (IData)((IData)(vlSelfRef.__VstlFirstIteration)));
    vlSelfRef.__VstlFirstIteration = 0U;
#ifdef VL_DEBUG
    if (VL_UNLIKELY(vlSymsp->_vm_contextp__->debug())) {
        Vtop___024root___dump_triggers__stl(vlSelfRef.__VstlTriggered, "stl"s);
    }
#endif
}

VL_ATTR_COLD bool Vtop___024root___trigger_anySet__stl(const VlUnpacked<QData/*63:0*/, 1> &in);

#ifdef VL_DEBUG
VL_ATTR_COLD void Vtop___024root___dump_triggers__stl(const VlUnpacked<QData/*63:0*/, 1> &triggers, const std::string &tag) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vtop___024root___dump_triggers__stl\n"); );
    // Body
    if ((1U & (~ (IData)(Vtop___024root___trigger_anySet__stl(triggers))))) {
        VL_DBG_MSGS("         No '" + tag + "' region triggers active\n");
    }
    if ((1U & (IData)(triggers[0U]))) {
        VL_DBG_MSGS("         '" + tag + "' region trigger index 0 is active: Internal 'stl' trigger - first iteration\n");
    }
}
#endif  // VL_DEBUG

VL_ATTR_COLD bool Vtop___024root___trigger_anySet__stl(const VlUnpacked<QData/*63:0*/, 1> &in) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vtop___024root___trigger_anySet__stl\n"); );
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

VL_ATTR_COLD void Vtop___024root___stl_sequent__TOP__0(Vtop___024root* vlSelf) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vtop___024root___stl_sequent__TOP__0\n"); );
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
    if ((vlSelfRef.top__DOT__sub__DOT__count_f ^ vlSelfRef.top__DOT__sub__DOT____Vtogcov__count_f)) {
        VL_COV_TOGGLE_CHG_ST_I(32, vlSymsp->__Vcoverage + 461, vlSelfRef.top__DOT__sub__DOT__count_f, vlSelfRef.top__DOT__sub__DOT____Vtogcov__count_f);
        vlSelfRef.top__DOT__sub__DOT____Vtogcov__count_f 
            = vlSelfRef.top__DOT__sub__DOT__count_f;
    }
    if ((vlSelfRef.top__DOT__sub__DOT__count_c ^ vlSelfRef.top__DOT__sub__DOT____Vtogcov__count_c)) {
        VL_COV_TOGGLE_CHG_ST_I(32, vlSymsp->__Vcoverage + 530, vlSelfRef.top__DOT__sub__DOT__count_c, vlSelfRef.top__DOT__sub__DOT____Vtogcov__count_c);
        vlSelfRef.top__DOT__sub__DOT____Vtogcov__count_c 
            = vlSelfRef.top__DOT__sub__DOT__count_c;
    }
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

VL_ATTR_COLD void Vtop___024root____Vm_traceActivitySetAll(Vtop___024root* vlSelf);

VL_ATTR_COLD void Vtop___024root___eval_stl(Vtop___024root* vlSelf) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vtop___024root___eval_stl\n"); );
    Vtop__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    auto& vlSelfRef = std::ref(*vlSelf).get();
    // Body
    if ((1ULL & vlSelfRef.__VstlTriggered[0U])) {
        Vtop___024root___stl_sequent__TOP__0(vlSelf);
        Vtop___024root____Vm_traceActivitySetAll(vlSelf);
    }
}

VL_ATTR_COLD bool Vtop___024root___eval_phase__stl(Vtop___024root* vlSelf) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vtop___024root___eval_phase__stl\n"); );
    Vtop__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    auto& vlSelfRef = std::ref(*vlSelf).get();
    // Locals
    CData/*0:0*/ __VstlExecute;
    // Body
    Vtop___024root___eval_triggers__stl(vlSelf);
    __VstlExecute = Vtop___024root___trigger_anySet__stl(vlSelfRef.__VstlTriggered);
    if (__VstlExecute) {
        Vtop___024root___eval_stl(vlSelf);
    }
    return (__VstlExecute);
}

bool Vtop___024root___trigger_anySet__ico(const VlUnpacked<QData/*63:0*/, 1> &in);

#ifdef VL_DEBUG
VL_ATTR_COLD void Vtop___024root___dump_triggers__ico(const VlUnpacked<QData/*63:0*/, 1> &triggers, const std::string &tag) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vtop___024root___dump_triggers__ico\n"); );
    // Body
    if ((1U & (~ (IData)(Vtop___024root___trigger_anySet__ico(triggers))))) {
        VL_DBG_MSGS("         No '" + tag + "' region triggers active\n");
    }
    if ((1U & (IData)(triggers[0U]))) {
        VL_DBG_MSGS("         '" + tag + "' region trigger index 0 is active: Internal 'ico' trigger - first iteration\n");
    }
}
#endif  // VL_DEBUG

bool Vtop___024root___trigger_anySet__act(const VlUnpacked<QData/*63:0*/, 1> &in);

#ifdef VL_DEBUG
VL_ATTR_COLD void Vtop___024root___dump_triggers__act(const VlUnpacked<QData/*63:0*/, 1> &triggers, const std::string &tag) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vtop___024root___dump_triggers__act\n"); );
    // Body
    if ((1U & (~ (IData)(Vtop___024root___trigger_anySet__act(triggers))))) {
        VL_DBG_MSGS("         No '" + tag + "' region triggers active\n");
    }
    if ((1U & (IData)(triggers[0U]))) {
        VL_DBG_MSGS("         '" + tag + "' region trigger index 0 is active: @(posedge __Vcellinp__top__fastclk)\n");
    }
    if ((1U & (IData)((triggers[0U] >> 1U)))) {
        VL_DBG_MSGS("         '" + tag + "' region trigger index 1 is active: @(posedge __Vcellinp__top__clk)\n");
    }
}
#endif  // VL_DEBUG

VL_ATTR_COLD void Vtop___024root____Vm_traceActivitySetAll(Vtop___024root* vlSelf) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vtop___024root____Vm_traceActivitySetAll\n"); );
    Vtop__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    auto& vlSelfRef = std::ref(*vlSelf).get();
    // Body
    vlSelfRef.__Vm_traceActivity[0U] = 1U;
    vlSelfRef.__Vm_traceActivity[1U] = 1U;
}

VL_ATTR_COLD void Vtop___024root___ctor_var_reset(Vtop___024root* vlSelf) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vtop___024root___ctor_var_reset\n"); );
    Vtop__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    auto& vlSelfRef = std::ref(*vlSelf).get();
    // Body
    VL_ZERO_RESET_W(70, vlSelf->__Vcellinp__top__in_wide);
    vlSelf->__Vcellinp__top__in_quad = 0;
    vlSelf->__Vcellinp__top__in_small = 0;
    VL_ZERO_RESET_W(70, vlSelf->__Vcellout__top__out_wide);
    vlSelf->__Vcellout__top__out_quad = 0;
    vlSelf->__Vcellout__top__out_small = 0;
    vlSelf->__Vcellinp__top__reset_l = 0;
    vlSelf->__Vcellinp__top__fastclk = 0;
    vlSelf->__Vcellinp__top__clk = 0;
    vlSelf->top__DOT____Vtogcov__clk = 0;
    vlSelf->top__DOT____Vtogcov__fastclk = 0;
    vlSelf->top__DOT____Vtogcov__reset_l = 0;
    vlSelf->top__DOT____Vtogcov__out_small = 0;
    vlSelf->top__DOT____Vtogcov__out_quad = 0;
    VL_ZERO_RESET_W(70, vlSelf->top__DOT____Vtogcov__out_wide);
    vlSelf->top__DOT____Vtogcov__in_small = 0;
    vlSelf->top__DOT____Vtogcov__in_quad = 0;
    VL_ZERO_RESET_W(70, vlSelf->top__DOT____Vtogcov__in_wide);
    const uint64_t __VscopeHash = VL_MURMUR64_HASH(vlSelf->vlNamep);
    vlSelf->top__DOT__sub__DOT__count_f = VL_SCOPED_RAND_RESET_I(32, __VscopeHash, 111824779708019218ull);
    vlSelf->top__DOT__sub__DOT__count_c = VL_SCOPED_RAND_RESET_I(32, __VscopeHash, 2802051788925154131ull);
    vlSelf->top__DOT__sub__DOT____Vtogcov__count_f = 0;
    vlSelf->top__DOT__sub__DOT____Vtogcov__count_c = 0;
    for (int __Vi0 = 0; __Vi0 < 1; ++__Vi0) {
        vlSelf->__VstlTriggered[__Vi0] = 0;
    }
    for (int __Vi0 = 0; __Vi0 < 1; ++__Vi0) {
        vlSelf->__VicoTriggered[__Vi0] = 0;
    }
    for (int __Vi0 = 0; __Vi0 < 1; ++__Vi0) {
        vlSelf->__VactTriggered[__Vi0] = 0;
    }
    vlSelf->__Vtrigprevexpr___TOP____Vcellinp__top__fastclk__0 = 0;
    vlSelf->__Vtrigprevexpr___TOP____Vcellinp__top__clk__0 = 0;
    for (int __Vi0 = 0; __Vi0 < 1; ++__Vi0) {
        vlSelf->__VnbaTriggered[__Vi0] = 0;
    }
    for (int __Vi0 = 0; __Vi0 < 2; ++__Vi0) {
        vlSelf->__Vm_traceActivity[__Vi0] = 0;
    }
}

VL_ATTR_COLD void Vtop___024root___configure_coverage(Vtop___024root* vlSelf, bool first) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vtop___024root___configure_coverage\n"); );
    Vtop__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    auto& vlSelfRef = std::ref(*vlSelf).get();
    // Body
    (void)first;  // Prevent unused variable warning
    vlSelf->__vlCoverToggleInsert(0, 0, 0, &(vlSymsp->__Vcoverage[0]), first, "top.v", 14, 23, ".top", "v_toggle/top", "clk");
    vlSelf->__vlCoverToggleInsert(0, 0, 0, &(vlSymsp->__Vcoverage[2]), first, "top.v", 15, 23, ".top", "v_toggle/top", "fastclk");
    vlSelf->__vlCoverToggleInsert(0, 0, 0, &(vlSymsp->__Vcoverage[4]), first, "top.v", 16, 23, ".top", "v_toggle/top", "reset_l");
    vlSelf->__vlCoverToggleInsert(0, 1, 1, &(vlSymsp->__Vcoverage[6]), first, "top.v", 18, 23, ".top", "v_toggle/top", "out_small");
    vlSelf->__vlCoverToggleInsert(0, 39, 1, &(vlSymsp->__Vcoverage[10]), first, "top.v", 19, 23, ".top", "v_toggle/top", "out_quad");
    vlSelf->__vlCoverToggleInsert(0, 69, 1, &(vlSymsp->__Vcoverage[90]), first, "top.v", 20, 23, ".top", "v_toggle/top", "out_wide");
    vlSelf->__vlCoverToggleInsert(0, 1, 1, &(vlSymsp->__Vcoverage[230]), first, "top.v", 21, 23, ".top", "v_toggle/top", "in_small");
    vlSelf->__vlCoverToggleInsert(0, 39, 1, &(vlSymsp->__Vcoverage[234]), first, "top.v", 22, 23, ".top", "v_toggle/top", "in_quad");
    vlSelf->__vlCoverToggleInsert(0, 69, 1, &(vlSymsp->__Vcoverage[314]), first, "top.v", 23, 23, ".top", "v_toggle/top", "in_wide");
    vlSelf->__vlCoverInsert(&(vlSymsp->__Vcoverage[454]), first, "top.v", 27, 33, ".top", "v_branch/top", "cond_then", "27");
    vlSelf->__vlCoverInsert(&(vlSymsp->__Vcoverage[455]), first, "top.v", 27, 34, ".top", "v_branch/top", "cond_else", "27");
    vlSelf->__vlCoverInsert(&(vlSymsp->__Vcoverage[456]), first, "top.v", 28, 33, ".top", "v_branch/top", "cond_then", "28");
    vlSelf->__vlCoverInsert(&(vlSymsp->__Vcoverage[457]), first, "top.v", 28, 34, ".top", "v_branch/top", "cond_else", "28");
    vlSelf->__vlCoverInsert(&(vlSymsp->__Vcoverage[458]), first, "top.v", 29, 33, ".top", "v_branch/top", "cond_then", "29");
    vlSelf->__vlCoverInsert(&(vlSymsp->__Vcoverage[459]), first, "top.v", 29, 34, ".top", "v_branch/top", "cond_else", "29");
    vlSelf->__vlCoverInsert(&(vlSymsp->__Vcoverage[460]), first, "top.v", 39, 3, ".top", "v_line/top", "block", "39-40");
    vlSelf->__vlCoverToggleInsert(0, 0, 0, &(vlSymsp->__Vcoverage[0]), first, "sub.v", 10, 10, ".top.sub", "v_toggle/sub", "clk");
    vlSelf->__vlCoverToggleInsert(0, 0, 0, &(vlSymsp->__Vcoverage[2]), first, "sub.v", 11, 10, ".top.sub", "v_toggle/sub", "fastclk");
    vlSelf->__vlCoverToggleInsert(0, 0, 0, &(vlSymsp->__Vcoverage[4]), first, "sub.v", 12, 10, ".top.sub", "v_toggle/sub", "reset_l");
    vlSelf->__vlCoverToggleInsert(0, 31, 1, &(vlSymsp->__Vcoverage[461]), first, "sub.v", 16, 14, ".top.sub", "v_toggle/sub", "count_f");
    vlSelf->__vlCoverInsert(&(vlSymsp->__Vcoverage[525]), first, "sub.v", 18, 5, ".top.sub", "v_branch/sub", "if", "18,21");
    vlSelf->__vlCoverInsert(&(vlSymsp->__Vcoverage[526]), first, "sub.v", 18, 6, ".top.sub", "v_branch/sub", "else", "24-25");
    vlSelf->__vlCoverInsert(&(vlSymsp->__Vcoverage[527]), first, "sub.v", 18, 9, ".top.sub", "v_expr/sub", "(reset_l==0) => 1", "");
    vlSelf->__vlCoverInsert(&(vlSymsp->__Vcoverage[528]), first, "sub.v", 18, 9, ".top.sub", "v_expr/sub", "(reset_l==1) => 0", "");
    vlSelf->__vlCoverInsert(&(vlSymsp->__Vcoverage[529]), first, "sub.v", 17, 3, ".top.sub", "v_line/sub", "block", "17");
    vlSelf->__vlCoverToggleInsert(0, 31, 1, &(vlSymsp->__Vcoverage[530]), first, "sub.v", 30, 14, ".top.sub", "v_toggle/sub", "count_c");
    vlSelf->__vlCoverInsert(&(vlSymsp->__Vcoverage[594]), first, "sub.v", 40, 7, ".top.sub", "v_branch/sub", "if", "40-41,44-45");
    vlSelf->__vlCoverInsert(&(vlSymsp->__Vcoverage[595]), first, "sub.v", 40, 8, ".top.sub", "v_branch/sub", "else", "");
    vlSelf->__vlCoverInsert(&(vlSymsp->__Vcoverage[596]), first, "sub.v", 32, 5, ".top.sub", "v_branch/sub", "if", "32,35");
    vlSelf->__vlCoverInsert(&(vlSymsp->__Vcoverage[597]), first, "sub.v", 32, 6, ".top.sub", "v_branch/sub", "else", "38-39");
    vlSelf->__vlCoverInsert(&(vlSymsp->__Vcoverage[598]), first, "sub.v", 32, 9, ".top.sub", "v_expr/sub", "(reset_l==0) => 1", "");
    vlSelf->__vlCoverInsert(&(vlSymsp->__Vcoverage[599]), first, "sub.v", 32, 9, ".top.sub", "v_expr/sub", "(reset_l==1) => 0", "");
    vlSelf->__vlCoverInsert(&(vlSymsp->__Vcoverage[600]), first, "sub.v", 31, 3, ".top.sub", "v_line/sub", "block", "31");
    vlSelf->__vlCoverInsert(&(vlSymsp->__Vcoverage[601]), first, "sub.v", 52, 41, ".top.sub", "v_expr/sub", "((count_c < 32'sh64)==1) => 1", "");
    vlSelf->__vlCoverInsert(&(vlSymsp->__Vcoverage[602]), first, "sub.v", 52, 41, ".top.sub", "v_expr/sub", "(reset_l==0) => 1", "");
    vlSelf->__vlCoverInsert(&(vlSymsp->__Vcoverage[603]), first, "sub.v", 52, 41, ".top.sub", "v_expr/sub", "(reset_l==1 && (count_c < 32'sh64)==0) => 0", "");
    vlSelf->__vlCoverInsert(&(vlSymsp->__Vcoverage[604]), first, "sub.v", 51, 3, ".top.sub", "v_line/sub", "block", "51-52");
    vlSelf->__vlCoverInsert(&(vlSymsp->__Vcoverage[605]), first, "sub.v", 56, 3, ".top.sub", "v_user/sub", "cover", "56");
}
