// Verilated -*- C++ -*-
// DESCRIPTION: Verilator output: Design implementation internals
// See Vexample.h for the primary calling header

#include "Vexample__pch.h"

void Vexample_std__03a__03aprocess__Vclpkg::__VnoInFunc_self(VlProcessRef vlProcess, Vexample__Syms* __restrict vlSymsp, VlClassRef<Vexample_std__03a__03aprocess> &self__Vfuncrtn) {
    VL_DEBUG_IF(VL_DBG_MSGF("+  Vexample_std__03a__03aprocess__Vclpkg::__VnoInFunc_self\n"); );
    // Body
    VlClassRef<Vexample_std__03a__03aprocess> p;
    p = VL_NEW(Vexample_std__03a__03aprocess, vlSymsp);

// $c statement at /home/ant/verilator-infrastructure-verilator4/include/verilated_std.sv:144:7
    VL_NULL_CHECK(p, "/home/ant/verilator-infrastructure-verilator4/include/verilated_std.sv", 144)->__PVT__m_process = vlProcess;
    self__Vfuncrtn = p;
}

void Vexample_std__03a__03aprocess__Vclpkg::__VnoInFunc_killQueue(Vexample__Syms* __restrict vlSymsp, VlQueue<VlClassRef<Vexample_std__03a__03aprocess>> &processQueue) {
    VL_DEBUG_IF(VL_DBG_MSGF("+  Vexample_std__03a__03aprocess__Vclpkg::__VnoInFunc_killQueue\n"); );
    // Body
    while (VL_LTS_III(32, 0U, processQueue.size())) {
        VL_NULL_CHECK(processQueue.pop_back(), "/home/ant/verilator-infrastructure-verilator4/include/verilated_std.sv", 184)->__VnoInFunc_kill(vlSymsp);
    }
}

void Vexample_std__03a__03aprocess::__VnoInFunc_set_status(Vexample__Syms* __restrict vlSymsp, IData/*31:0*/ s) {
    VL_DEBUG_IF(VL_DBG_MSGF("+            Vexample_std__03a__03aprocess::__VnoInFunc_set_status\n"); );
    // Body

// $c statement at /home/ant/verilator-infrastructure-verilator4/include/verilated_std.sv:151:7
    this->__PVT__m_process->state(s);
}

void Vexample_std__03a__03aprocess::__VnoInFunc_status(Vexample__Syms* __restrict vlSymsp, IData/*31:0*/ &status__Vfuncrtn) {
    VL_DEBUG_IF(VL_DBG_MSGF("+            Vexample_std__03a__03aprocess::__VnoInFunc_status\n"); );
    // Body
    status__Vfuncrtn = 
// $cpure expression at /home/ant/verilator-infrastructure-verilator4/include/verilated_std.sv:157:21
this->__PVT__m_process->state()
    ;
}

void Vexample_std__03a__03aprocess::__VnoInFunc_kill(Vexample__Syms* __restrict vlSymsp) {
    VL_DEBUG_IF(VL_DBG_MSGF("+            Vexample_std__03a__03aprocess::__VnoInFunc_kill\n"); );
    // Body
    this->__VnoInFunc_set_status(vlSymsp, 4U);
}

void Vexample_std__03a__03aprocess::__VnoInFunc_suspend(Vexample__Syms* __restrict vlSymsp) {
    VL_DEBUG_IF(VL_DBG_MSGF("+            Vexample_std__03a__03aprocess::__VnoInFunc_suspend\n"); );
    // Body
    VL_WRITEF_NX("[%0t] %%Error: verilated_std.sv:168: Assertion failed in %Nstd.process.suspend: std::process::suspend() not supported\n",0,
                 64,VL_TIME_UNITED_Q(1),-12,vlSymsp->name());
    VL_STOP_MT("/home/ant/verilator-infrastructure-verilator4/include/verilated_std.sv", 168, "");
}

void Vexample_std__03a__03aprocess::__VnoInFunc_resume(Vexample__Syms* __restrict vlSymsp) {
    VL_DEBUG_IF(VL_DBG_MSGF("+            Vexample_std__03a__03aprocess::__VnoInFunc_resume\n"); );
    // Body
    this->__VnoInFunc_set_status(vlSymsp, 1U);
}

VlCoroutine Vexample_std__03a__03aprocess::__VnoInFunc_await(Vexample__Syms* __restrict vlSymsp) {
    VL_DEBUG_IF(VL_DBG_MSGF("+            Vexample_std__03a__03aprocess::__VnoInFunc_await\n"); );
    // Locals
    IData/*31:0*/ __Vfunc_status__2__Vfuncout;
    __Vfunc_status__2__Vfuncout = 0;
    IData/*31:0*/ __Vfunc_status__3__Vfuncout;
    __Vfunc_status__3__Vfuncout = 0;
    // Body
    VL_KEEP_THIS;
    if ((1U & (~ ((0U == ([&]() {
                                this->__VnoInFunc_status(vlSymsp, __Vfunc_status__2__Vfuncout);
                            }(), __Vfunc_status__2__Vfuncout)) 
                  | (4U == ([&]() {
                                this->__VnoInFunc_status(vlSymsp, __Vfunc_status__3__Vfuncout);
                            }(), __Vfunc_status__3__Vfuncout)))))) {
        CData/*0:0*/ __VdynTrigger_h794f2230__0;
        __VdynTrigger_h794f2230__0 = 0;
        __VdynTrigger_h794f2230__0 = 0U;
        while ((1U & (~ (IData)(__VdynTrigger_h794f2230__0)))) {
            co_await vlSymsp->TOP.__VdynSched.evaluation(
                                                         nullptr, 
                                                         "@([true] ((32'h0 == $_EXPRSTMT( // Function: status __VnoInFunc_status(std::process.__Vfunc_status__2__Vfuncout); , ); ) | (32'h4 == $_EXPRSTMT( // Function: status __VnoInFunc_status(std::process.__Vfunc_status__3__Vfuncout); , ); )))", 
                                                         "/home/ant/verilator-infrastructure-verilator4/include/verilated_std.sv", 
                                                         177);
            this->__Vtrigprevexpr_h4b30777f__0 = ((0U 
                                                   == 
                                                   ([&]() {
                            this->__VnoInFunc_status(vlSymsp, __Vfunc_status__2__Vfuncout);
                        }(), __Vfunc_status__2__Vfuncout)) 
                                                  | (4U 
                                                     == 
                                                     ([&]() {
                            this->__VnoInFunc_status(vlSymsp, __Vfunc_status__3__Vfuncout);
                        }(), __Vfunc_status__3__Vfuncout)));
            __VdynTrigger_h794f2230__0 = this->__Vtrigprevexpr_h4b30777f__0;
            vlSymsp->TOP.__VdynSched.anyTriggered(__VdynTrigger_h794f2230__0);
        }
        co_await vlSymsp->TOP.__VdynSched.resumption(
                                                     nullptr, 
                                                     "@([true] ((32'h0 == $_EXPRSTMT( // Function: status __VnoInFunc_status(std::process.__Vfunc_status__2__Vfuncout); , ); ) | (32'h4 == $_EXPRSTMT( // Function: status __VnoInFunc_status(std::process.__Vfunc_status__3__Vfuncout); , ); )))", 
                                                     "/home/ant/verilator-infrastructure-verilator4/include/verilated_std.sv", 
                                                     177);
    }
    co_return;}

void Vexample_std__03a__03aprocess::__VnoInFunc_get_randstate(Vexample__Syms* __restrict vlSymsp, std::string &get_randstate__Vfuncrtn) {
    VL_DEBUG_IF(VL_DBG_MSGF("+            Vexample_std__03a__03aprocess::__VnoInFunc_get_randstate\n"); );
    // Body
    std::string s;
    s = VL_CVT_PACK_STR_NI(
// $c expression at /home/ant/verilator-infrastructure-verilator4/include/verilated_std.sv:230:26
0
    );

// $c statement at /home/ant/verilator-infrastructure-verilator4/include/verilated_std.sv:232:7
    s = this->__PVT__m_process->randstate();
    get_randstate__Vfuncrtn = s;
}

void Vexample_std__03a__03aprocess::__VnoInFunc_set_randstate(Vexample__Syms* __restrict vlSymsp, std::string s) {
    VL_DEBUG_IF(VL_DBG_MSGF("+            Vexample_std__03a__03aprocess::__VnoInFunc_set_randstate\n"); );
    // Body

// $c statement at /home/ant/verilator-infrastructure-verilator4/include/verilated_std.sv:237:7
    this->__PVT__m_process->randstate(s);
}

Vexample_std__03a__03aprocess::Vexample_std__03a__03aprocess(Vexample__Syms* __restrict vlSymsp) {
    VL_DEBUG_IF(VL_DBG_MSGF("+            Vexample_std__03a__03aprocess::new\n"); );
    // Body
    _ctor_var_reset(vlSymsp);
}

void Vexample_std__03a__03aprocess::_ctor_var_reset(Vexample__Syms* __restrict vlSymsp) {
    VL_DEBUG_IF(VL_DBG_MSGF("+            Vexample_std__03a__03aprocess::_ctor_var_reset\n"); );
    // Body
    (void)vlSymsp;  // Prevent unused variable warning
    __Vtrigprevexpr_h4b30777f__0 = 0;
}

std::string VL_TO_STRING(const VlClassRef<Vexample_std__03a__03aprocess>& obj) {
    VL_DEBUG_IF(VL_DBG_MSGF("+            Vexample_std__03a__03aprocess::VL_TO_STRING\n"); );
    // Body
    return (obj ? obj->to_string() : "null");
}

std::string Vexample_std__03a__03aprocess::to_string() const {
    VL_DEBUG_IF(VL_DBG_MSGF("+            Vexample_std__03a__03aprocess::to_string\n"); );
    // Body
    return ("'{"s + to_string_middle() + "}");
}

std::string Vexample_std__03a__03aprocess::to_string_middle() const {
    VL_DEBUG_IF(VL_DBG_MSGF("+            Vexample_std__03a__03aprocess::to_string_middle\n"); );
    // Body
    std::string out;
    out += "m_process:" + VL_TO_STRING(__PVT__m_process);
    return (out);
}
