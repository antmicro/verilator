// Verilated -*- C++ -*-
// DESCRIPTION: Verilator output: Model implementation (design independent parts)

#include "Vexample__pch.h"

//============================================================
// Constructors

Vexample::Vexample(VerilatedContext* _vcontextp__, const char* _vcname__)
    : VerilatedModel{*_vcontextp__}
    , vlSymsp{new Vexample__Syms(contextp(), _vcname__, this)}
    , __PVT____024unit{vlSymsp->TOP.__PVT____024unit}
    , __PVT__std{vlSymsp->TOP.__PVT__std}
    , __024unit__03a__03aBase__Vclpkg{vlSymsp->TOP.__024unit__03a__03aBase__Vclpkg}
    , __024unit__03a__03aDerived__Vclpkg{vlSymsp->TOP.__024unit__03a__03aDerived__Vclpkg}
    , std__03a__03asemaphore__Vclpkg{vlSymsp->TOP.std__03a__03asemaphore__Vclpkg}
    , std__03a__03aprocess__Vclpkg{vlSymsp->TOP.std__03a__03aprocess__Vclpkg}
    , rootp{&(vlSymsp->TOP)}
{
    // Register model with the context
    contextp()->addModel(this);
}

Vexample::Vexample(const char* _vcname__)
    : Vexample(Verilated::threadContextp(), _vcname__)
{
}

//============================================================
// Destructor

Vexample::~Vexample() {
    delete vlSymsp;
}

//============================================================
// Evaluation function

#ifdef VL_DEBUG
void Vexample___024root___eval_debug_assertions(Vexample___024root* vlSelf);
#endif  // VL_DEBUG
void Vexample___024root___eval_static(Vexample___024root* vlSelf);
void Vexample___024root___eval_initial(Vexample___024root* vlSelf);
void Vexample___024root___eval_settle(Vexample___024root* vlSelf);
void Vexample___024root___eval(Vexample___024root* vlSelf);

void Vexample::eval_step() {
    VL_DEBUG_IF(VL_DBG_MSGF("+++++TOP Evaluate Vexample::eval_step\n"); );
#ifdef VL_DEBUG
    // Debug assertions
    Vexample___024root___eval_debug_assertions(&(vlSymsp->TOP));
#endif  // VL_DEBUG
    vlSymsp->__Vm_deleter.deleteAll();
    if (VL_UNLIKELY(!vlSymsp->__Vm_didInit)) {
        vlSymsp->__Vm_didInit = true;
        VL_DEBUG_IF(VL_DBG_MSGF("+ Initial\n"););
        Vexample___024root___eval_static(&(vlSymsp->TOP));
        Vexample___024root___eval_initial(&(vlSymsp->TOP));
        Vexample___024root___eval_settle(&(vlSymsp->TOP));
    }
    VL_DEBUG_IF(VL_DBG_MSGF("+ Eval\n"););
    Vexample___024root___eval(&(vlSymsp->TOP));
    // Evaluate cleanup
    Verilated::endOfEval(vlSymsp->__Vm_evalMsgQp);
}

//============================================================
// Events and timing
bool Vexample::eventsPending() { return !vlSymsp->TOP.__VdlySched.empty(); }

uint64_t Vexample::nextTimeSlot() { return vlSymsp->TOP.__VdlySched.nextTimeSlot(); }

//============================================================
// Utilities

const char* Vexample::name() const {
    return vlSymsp->name();
}

//============================================================
// Invoke final blocks

void Vexample___024root___eval_final(Vexample___024root* vlSelf);

VL_ATTR_COLD void Vexample::final() {
    Vexample___024root___eval_final(&(vlSymsp->TOP));
}

//============================================================
// Implementations of abstract methods from VerilatedModel

const char* Vexample::hierName() const { return vlSymsp->name(); }
const char* Vexample::modelName() const { return "Vexample"; }
unsigned Vexample::threads() const { return 1; }
void Vexample::prepareClone() const { contextp()->prepareClone(); }
void Vexample::atClone() const {
    contextp()->threadPoolpOnClone();
}
