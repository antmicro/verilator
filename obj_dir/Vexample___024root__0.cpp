// Verilated -*- C++ -*-
// DESCRIPTION: Verilator output: Design implementation internals
// See Vexample.h for the primary calling header

#include "Vexample__pch.h"

VlCoroutine Vexample___024root___eval_initial__TOP__Vtiming__0(Vexample___024root* vlSelf, VlProcessRef vlProcess);

void Vexample___024root___eval_initial(Vexample___024root* vlSelf) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vexample___024root___eval_initial\n"); );
    Vexample__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    auto& vlSelfRef = std::ref(*vlSelf).get();
    // Body
    Vexample___024root___eval_initial__TOP__Vtiming__0(vlSelf, std::make_shared<VlProcess>());
}

VlCoroutine Vexample___024root___eval_initial__TOP__Vtiming__0(Vexample___024root* vlSelf, VlProcessRef vlProcess) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vexample___024root___eval_initial__TOP__Vtiming__0\n"); );
    Vexample__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    auto& vlSelfRef = std::ref(*vlSelf).get();
    // Locals
    VlClassRef<Vexample___024unit__03a__03aDerived> t__DOT__d;
    // Body
    t__DOT__d = VL_NEW(Vexample___024unit__03a__03aDerived, vlProcess, vlSymsp);
    co_await vlSelfRef.__VdlySched.delay(1ULL, vlProcess, 
                                         "example.sv", 
                                         28);
    vlProcess->state(VlProcess::FINISHED);
    co_return;}

#ifdef VL_DEBUG
VL_ATTR_COLD void Vexample___024root___dump_triggers__act(const VlUnpacked<QData/*63:0*/, 1> &triggers, const std::string &tag);
#endif  // VL_DEBUG

void Vexample___024root___eval_triggers__act(Vexample___024root* vlSelf) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vexample___024root___eval_triggers__act\n"); );
    Vexample__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    auto& vlSelfRef = std::ref(*vlSelf).get();
    // Body
    vlSelfRef.__VactTriggered[0U] = VL_EXTEND_QI(64,2, 
                                                 ((vlSelfRef.__VdynSched.evaluate() 
                                                   << 1U) 
                                                  | vlSelfRef.__VdlySched.awaitingCurrentTime()));
#ifdef VL_DEBUG
    if (VL_UNLIKELY(vlSymsp->_vm_contextp__->debug())) {
        Vexample___024root___dump_triggers__act(vlSelfRef.__VactTriggered, "act"s);
    }
#endif
    vlSelfRef.__VdynSched.doPostUpdates();
}

bool Vexample___024root___trigger_anySet__act(const VlUnpacked<QData/*63:0*/, 1> &in) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vexample___024root___trigger_anySet__act\n"); );
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

void Vexample___024root___timing_resume(Vexample___024root* vlSelf) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vexample___024root___timing_resume\n"); );
    Vexample__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    auto& vlSelfRef = std::ref(*vlSelf).get();
    // Body
    if ((2ULL & vlSelfRef.__VactTriggered[0U])) {
        vlSelfRef.__VdynSched.resume();
    }
    if ((1ULL & vlSelfRef.__VactTriggered[0U])) {
        vlSelfRef.__VdlySched.resume();
    }
}

void Vexample___024root___trigger_orInto__act(VlUnpacked<QData/*63:0*/, 1> &out, const VlUnpacked<QData/*63:0*/, 1> &in) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vexample___024root___trigger_orInto__act\n"); );
    // Locals
    IData/*31:0*/ n;
    // Body
    n = 0U;
    do {
        out[n] = (out[n] | in[n]);
        n = ((IData)(1U) + n);
    } while ((1U > n));
}

bool Vexample___024root___eval_phase__act(Vexample___024root* vlSelf) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vexample___024root___eval_phase__act\n"); );
    Vexample__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    auto& vlSelfRef = std::ref(*vlSelf).get();
    // Locals
    CData/*0:0*/ __VactExecute;
    // Body
    Vexample___024root___eval_triggers__act(vlSelf);
    Vexample___024root___trigger_orInto__act(vlSelfRef.__VnbaTriggered, vlSelfRef.__VactTriggered);
    __VactExecute = Vexample___024root___trigger_anySet__act(vlSelfRef.__VactTriggered);
    if (__VactExecute) {
        Vexample___024root___timing_resume(vlSelf);
    }
    return (__VactExecute);
}

void Vexample___024root___trigger_clear__act(VlUnpacked<QData/*63:0*/, 1> &out) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vexample___024root___trigger_clear__act\n"); );
    // Locals
    IData/*31:0*/ n;
    // Body
    n = 0U;
    do {
        out[n] = 0ULL;
        n = ((IData)(1U) + n);
    } while ((1U > n));
}

bool Vexample___024root___eval_phase__nba(Vexample___024root* vlSelf) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vexample___024root___eval_phase__nba\n"); );
    Vexample__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    auto& vlSelfRef = std::ref(*vlSelf).get();
    // Locals
    CData/*0:0*/ __VnbaExecute;
    // Body
    __VnbaExecute = Vexample___024root___trigger_anySet__act(vlSelfRef.__VnbaTriggered);
    if (__VnbaExecute) {
        Vexample___024root___trigger_clear__act(vlSelfRef.__VnbaTriggered);
    }
    return (__VnbaExecute);
}

void Vexample___024root___eval(Vexample___024root* vlSelf) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vexample___024root___eval\n"); );
    Vexample__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    auto& vlSelfRef = std::ref(*vlSelf).get();
    // Locals
    IData/*31:0*/ __VnbaIterCount;
    // Body
    __VnbaIterCount = 0U;
    do {
        if (VL_UNLIKELY(((0x00000064U < __VnbaIterCount)))) {
#ifdef VL_DEBUG
            Vexample___024root___dump_triggers__act(vlSelfRef.__VnbaTriggered, "nba"s);
#endif
            VL_FATAL_MT("example.sv", 23, "", "NBA region did not converge after 100 tries");
        }
        __VnbaIterCount = ((IData)(1U) + __VnbaIterCount);
        vlSelfRef.__VactIterCount = 0U;
        do {
            if (VL_UNLIKELY(((0x00000064U < vlSelfRef.__VactIterCount)))) {
#ifdef VL_DEBUG
                Vexample___024root___dump_triggers__act(vlSelfRef.__VactTriggered, "act"s);
#endif
                VL_FATAL_MT("example.sv", 23, "", "Active region did not converge after 100 tries");
            }
            vlSelfRef.__VactIterCount = ((IData)(1U) 
                                         + vlSelfRef.__VactIterCount);
        } while (Vexample___024root___eval_phase__act(vlSelf));
    } while (Vexample___024root___eval_phase__nba(vlSelf));
}

#ifdef VL_DEBUG
void Vexample___024root___eval_debug_assertions(Vexample___024root* vlSelf) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vexample___024root___eval_debug_assertions\n"); );
    Vexample__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    auto& vlSelfRef = std::ref(*vlSelf).get();
}
#endif  // VL_DEBUG
