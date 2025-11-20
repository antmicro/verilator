// Verilated -*- C++ -*-
// DESCRIPTION: Verilator output: Tracing implementation internals

#include "verilated_vcd_sc.h"
#include "Vtop__Syms.h"


void Vtop___024root__trace_chg_0_sub_0(Vtop___024root* vlSelf, VerilatedVcd::Buffer* bufp);

void Vtop___024root__trace_chg_0(void* voidSelf, VerilatedVcd::Buffer* bufp) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vtop___024root__trace_chg_0\n"); );
    // Body
    Vtop___024root* const __restrict vlSelf VL_ATTR_UNUSED = static_cast<Vtop___024root*>(voidSelf);
    Vtop__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    if (VL_UNLIKELY(!vlSymsp->__Vm_activity)) return;
    Vtop___024root__trace_chg_0_sub_0((&vlSymsp->TOP), bufp);
}

void Vtop___024root__trace_chg_0_sub_0(Vtop___024root* vlSelf, VerilatedVcd::Buffer* bufp) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vtop___024root__trace_chg_0_sub_0\n"); );
    Vtop__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    auto& vlSelfRef = std::ref(*vlSelf).get();
    // Body
    uint32_t* const oldp VL_ATTR_UNUSED = bufp->oldp(vlSymsp->__Vm_baseCode + 1);
    if (VL_UNLIKELY((vlSelfRef.__Vm_traceActivity[1U]))) {
        bufp->chgBit(oldp+0,(vlSelfRef.__Vcellinp__top__clk));
        bufp->chgBit(oldp+1,(vlSelfRef.__Vcellinp__top__fastclk));
        bufp->chgBit(oldp+2,(vlSelfRef.__Vcellinp__top__reset_l));
        bufp->chgCData(oldp+3,(vlSelfRef.__Vcellout__top__out_small),2);
        bufp->chgQData(oldp+4,(vlSelfRef.__Vcellout__top__out_quad),40);
        bufp->chgWData(oldp+6,(vlSelfRef.__Vcellout__top__out_wide),70);
        bufp->chgCData(oldp+9,(vlSelfRef.__Vcellinp__top__in_small),2);
        bufp->chgQData(oldp+10,(vlSelfRef.__Vcellinp__top__in_quad),40);
        bufp->chgWData(oldp+12,(vlSelfRef.__Vcellinp__top__in_wide),70);
    }
    bufp->chgIData(oldp+15,(vlSelfRef.top__DOT__sub__DOT__count_f),32);
    bufp->chgIData(oldp+16,(vlSelfRef.top__DOT__sub__DOT__count_c),32);
}

void Vtop___024root__trace_cleanup(void* voidSelf, VerilatedVcd* /*unused*/) {
    VL_DEBUG_IF(VL_DBG_MSGF("+    Vtop___024root__trace_cleanup\n"); );
    // Body
    Vtop___024root* const __restrict vlSelf VL_ATTR_UNUSED = static_cast<Vtop___024root*>(voidSelf);
    Vtop__Syms* const __restrict vlSymsp VL_ATTR_UNUSED = vlSelf->vlSymsp;
    vlSymsp->__Vm_activity = false;
    vlSymsp->TOP.__Vm_traceActivity[0U] = 0U;
    vlSymsp->TOP.__Vm_traceActivity[1U] = 0U;
}
