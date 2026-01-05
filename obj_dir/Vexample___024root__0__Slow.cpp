// Verilated -*- C++ -*-
// DESCRIPTION: Verilator output: Design implementation internals
// See Vexample.h for the primary calling header

#include "Vexample__pch.h"

VL_ATTR_COLD void Vexample___024root___eval_static(Vexample___024root* vlSelf) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vexample___024root___eval_static\n"); );
    Vexample__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    auto& vlSelfRef = std::ref(*vlSelf).get();
}

VL_ATTR_COLD void Vexample___024root___eval_final(Vexample___024root* vlSelf) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vexample___024root___eval_final\n"); );
    Vexample__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    auto& vlSelfRef = std::ref(*vlSelf).get();
}

VL_ATTR_COLD void Vexample___024root___eval_settle(Vexample___024root* vlSelf) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vexample___024root___eval_settle\n"); );
    Vexample__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    auto& vlSelfRef = std::ref(*vlSelf).get();
}

bool Vexample___024root___trigger_anySet__act(const VlUnpacked<QData/*63:0*/, 1> &in);

#ifdef VL_DEBUG
VL_ATTR_COLD void Vexample___024root___dump_triggers__act(const VlUnpacked<QData/*63:0*/, 1> &triggers, const std::string &tag) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vexample___024root___dump_triggers__act\n"); );
    // Body
    if ((1U & (~ (IData)(Vexample___024root___trigger_anySet__act(triggers))))) {
        VL_DBG_MSGS("         No '" + tag + "' region triggers active\n");
    }
    if ((1U & (IData)(triggers[0U]))) {
        VL_DBG_MSGS("         '" + tag + "' region trigger index 0 is active: @([true] __VdlySched.awaitingCurrentTime())\n");
    }
    if ((1U & (IData)((triggers[0U] >> 1U)))) {
        VL_DBG_MSGS("         '" + tag + "' region trigger index 1 is active: @([true] __VdynSched.evaluate())\n");
    }
}
#endif  // VL_DEBUG

VL_ATTR_COLD void Vexample___024root___ctor_var_reset(Vexample___024root* vlSelf) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vexample___024root___ctor_var_reset\n"); );
    Vexample__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    auto& vlSelfRef = std::ref(*vlSelf).get();
    // Body
    for (int __Vi0 = 0; __Vi0 < 1; ++__Vi0) {
        vlSelf->__VactTriggered[__Vi0] = 0;
    }
    for (int __Vi0 = 0; __Vi0 < 1; ++__Vi0) {
        vlSelf->__VnbaTriggered[__Vi0] = 0;
    }
}
