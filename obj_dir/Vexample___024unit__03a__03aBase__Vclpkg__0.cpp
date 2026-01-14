// Verilated -*- C++ -*-
// DESCRIPTION: Verilator output: Design implementation internals
// See Vexample.h for the primary calling header

#include "Vexample__pch.h"

Vexample___024unit__03a__03aBase::Vexample___024unit__03a__03aBase(VlProcessRef vlProcess, Vexample__Syms* __restrict vlSymsp, std::string foo) {
    VL_DEBUG_IF(VL_DBG_MSGF("+          Vexample___024unit__03a__03aBase::new\n"); );
    // Locals
    VlClassRef<Vexample_std__03a__03aprocess> __Vfunc_self__0__Vfuncout;
    // Body
    _ctor_var_reset(vlSymsp);
    this->__PVT__k = 7U;
    VlClassRef<Vexample_std__03a__03aprocess> unnamedblk1__DOT____VforkParent;
    VL_WRITEF_NX("%@\n",0,-1,&(foo));
    vlSymsp->TOP__std__03a__03aprocess__Vclpkg.__VnoInFunc_self(vlProcess, vlSymsp, __Vfunc_self__0__Vfuncout);
    unnamedblk1__DOT____VforkParent = __Vfunc_self__0__Vfuncout;
    this->new____Vfork_1__0(std::make_shared<VlProcess>(vlProcess), vlSymsp, unnamedblk1__DOT____VforkParent);
}

VlCoroutine Vexample___024unit__03a__03aBase::new____Vfork_1__0(VlProcessRef vlProcess, Vexample__Syms* __restrict vlSymsp, VlClassRef<Vexample_std__03a__03aprocess> unnamedblk1__DOT____VforkParent) {
    VL_DEBUG_IF(VL_DBG_MSGF("+          Vexample___024unit__03a__03aBase::new____Vfork_1__0\n"); );
    // Locals
    VlClassRef<Vexample_std__03a__03aprocess> __Vtask___VforkTask_0__1____VforkParent;
    IData/*31:0*/ __Vtask_status__2__Vfuncout;
    __Vtask_status__2__Vfuncout = 0;
    // Body
    VL_KEEP_THIS;
    __Vtask___VforkTask_0__1____VforkParent = unnamedblk1__DOT____VforkParent;
    if ((1U == ([&]() {
                    VL_NULL_CHECK(__Vtask___VforkTask_0__1____VforkParent, "example.sv", 8)
                ->__VnoInFunc_status(vlSymsp, __Vtask_status__2__Vfuncout);
                }(), __Vtask_status__2__Vfuncout))) {
        CData/*0:0*/ __VdynTrigger_hf707780e__0;
        __VdynTrigger_hf707780e__0 = 0;
        __VdynTrigger_hf707780e__0 = 0U;
        while ((1U & (~ (IData)(__VdynTrigger_hf707780e__0)))) {
            co_await vlSymsp->TOP.__VdynSched.evaluation(
                                                         vlProcess, 
                                                         "@([true] (32'h1 != $_EXPRSTMT( // Function: status $unit::Base.__Vtask___VforkTask_0__1____VforkParent.($unit::Base.__Vtask_status__2__Vfuncout); , ); ))", 
                                                         "example.sv", 
                                                         8);
            this->__Vtrigprevexpr_hcce7cd61__0 = (1U 
                                                  != 
                                                  ([&]() {
                        VL_NULL_CHECK(__Vtask___VforkTask_0__1____VforkParent, "example.sv", 8)
                                                   ->__VnoInFunc_status(vlSymsp, __Vtask_status__2__Vfuncout);
                    }(), __Vtask_status__2__Vfuncout));
            __VdynTrigger_hf707780e__0 = this->__Vtrigprevexpr_hcce7cd61__0;
            vlSymsp->TOP.__VdynSched.anyTriggered(__VdynTrigger_hf707780e__0);
        }
        co_await vlSymsp->TOP.__VdynSched.resumption(
                                                     vlProcess, 
                                                     "@([true] (32'h1 != $_EXPRSTMT( // Function: status $unit::Base.__Vtask___VforkTask_0__1____VforkParent.($unit::Base.__Vtask_status__2__Vfuncout); , ); ))", 
                                                     "example.sv", 
                                                     8);
    }
    VL_WRITEF_NX("Do some forking\n",0);
    vlProcess->state(VlProcess::FINISHED);
    co_return;}

void Vexample___024unit__03a__03aBase::__VnoInFunc_get_val(Vexample__Syms* __restrict vlSymsp, std::string &get_val__Vfuncrtn) {
    VL_DEBUG_IF(VL_DBG_MSGF("+          Vexample___024unit__03a__03aBase::__VnoInFunc_get_val\n"); );
    // Body
    VlClassRef<Vexample___024unit__03a__03aBase> b;
    b = VlClassRef<Vexample___024unit__03a__03aBase>{this};
    get_val__Vfuncrtn = "Hello World"s;
}

void Vexample___024unit__03a__03aBase::_ctor_var_reset(Vexample__Syms* __restrict vlSymsp) {
    VL_DEBUG_IF(VL_DBG_MSGF("+          Vexample___024unit__03a__03aBase::_ctor_var_reset\n"); );
    // Body
    (void)vlSymsp;  // Prevent unused variable warning
    __PVT__k = 0;
    __PVT__queue.atDefault() = 0;
    __Vtrigprevexpr_hcce7cd61__0 = 0;
}

Vexample___024unit__03a__03aBase::~Vexample___024unit__03a__03aBase() {
    VL_DEBUG_IF(VL_DBG_MSGF("+          Vexample___024unit__03a__03aBase::~\n"); );
}

std::string VL_TO_STRING(const VlClassRef<Vexample___024unit__03a__03aBase>& obj) {
    VL_DEBUG_IF(VL_DBG_MSGF("+          Vexample___024unit__03a__03aBase::VL_TO_STRING\n"); );
    // Body
    return (obj ? obj->to_string() : "null");
}

std::string Vexample___024unit__03a__03aBase::to_string() const {
    VL_DEBUG_IF(VL_DBG_MSGF("+          Vexample___024unit__03a__03aBase::to_string\n"); );
    // Body
    return ("'{"s + to_string_middle() + "}");
}

std::string Vexample___024unit__03a__03aBase::to_string_middle() const {
    VL_DEBUG_IF(VL_DBG_MSGF("+          Vexample___024unit__03a__03aBase::to_string_middle\n"); );
    // Body
    std::string out;
    out += "k:" + VL_TO_STRING(__PVT__k);
    out += ", queue:" + VL_TO_STRING(__PVT__queue);
    out += ", a:" + VL_TO_STRING(__PVT__a);
    return (out);
}
