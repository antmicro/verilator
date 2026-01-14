// Verilated -*- C++ -*-
// DESCRIPTION: Verilator output: Design implementation internals
// See Vexample.h for the primary calling header

#include "Vexample__pch.h"

Vexample___024unit__03a__03aDerived::Vexample___024unit__03a__03aDerived(VlProcessRef vlProcess, Vexample__Syms* __restrict vlSymsp)
    : Vexample___024unit__03a__03aBase(vlProcess, vlSymsp) {
    VL_DEBUG_IF(VL_DBG_MSGF("+          Vexample___024unit__03a__03aDerived::new\n"); );
}

void Vexample___024unit__03a__03aDerived::init(VlProcessRef vlProcess, Vexample__Syms* __restrict vlSymsp) {
    Vexample___024unit__03a__03aBase::init(vlProcess, vlSymsp, 
                                       VL_CVT_PACK_STR_NN(
                                                          ([&]() {
                std::string __Vfunc_get_val__1__Vfuncout;
                this->__VnoInFunc_get_val(vlSymsp, __Vfunc_get_val__1__Vfuncout);
                return (__Vfunc_get_val__1__Vfuncout);
            }())));
    // Body
    _ctor_var_reset(vlSymsp);
    this->__PVT__queue2 = VlQueue<IData/*31:0*/>::consVC(3U, 
                                                         VlQueue<IData/*31:0*/>::consVV(2U, 
                                                                                1U));
    ;
}

void Vexample___024unit__03a__03aDerived::_ctor_var_reset(Vexample__Syms* __restrict vlSymsp) {
    VL_DEBUG_IF(VL_DBG_MSGF("+          Vexample___024unit__03a__03aDerived::_ctor_var_reset\n"); );
    // Body
    (void)vlSymsp;  // Prevent unused variable warning
    __PVT__queue2.atDefault() = 0;
}

Vexample___024unit__03a__03aDerived::~Vexample___024unit__03a__03aDerived() {
    VL_DEBUG_IF(VL_DBG_MSGF("+          Vexample___024unit__03a__03aDerived::~\n"); );
}

std::string VL_TO_STRING(const VlClassRef<Vexample___024unit__03a__03aDerived>& obj) {
    VL_DEBUG_IF(VL_DBG_MSGF("+          Vexample___024unit__03a__03aDerived::VL_TO_STRING\n"); );
    // Body
    return (obj ? obj->to_string() : "null");
}

std::string Vexample___024unit__03a__03aDerived::to_string() const {
    VL_DEBUG_IF(VL_DBG_MSGF("+          Vexample___024unit__03a__03aDerived::to_string\n"); );
    // Body
    return ("'{"s + to_string_middle() + "}");
}

std::string Vexample___024unit__03a__03aDerived::to_string_middle() const {
    VL_DEBUG_IF(VL_DBG_MSGF("+          Vexample___024unit__03a__03aDerived::to_string_middle\n"); );
    // Body
    std::string out;
    out += "queue2:" + VL_TO_STRING(__PVT__queue2);
    out += ", b:" + VL_TO_STRING(__PVT__b);
    out += ", __Vfunc_get_val__1__Vfuncout:" + VL_TO_STRING(__Vfunc_get_val__1__Vfuncout);
    out += ", "+ Vexample___024unit__03a__03aBase::to_string_middle();
    return (out);
}
