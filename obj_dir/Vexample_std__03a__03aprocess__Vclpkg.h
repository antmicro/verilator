// Verilated -*- C++ -*-
// DESCRIPTION: Verilator output: Design internal header
// See Vexample.h for the primary calling header

#ifndef VERILATED_VEXAMPLE_STD__03A__03APROCESS__VCLPKG_H_
#define VERILATED_VEXAMPLE_STD__03A__03APROCESS__VCLPKG_H_  // guard

#include "verilated.h"
#include "verilated_timing.h"
class Vexample_std__03a__03aprocess;


class Vexample__Syms;

class alignas(VL_CACHE_LINE_BYTES) Vexample_std__03a__03aprocess__Vclpkg final {
  public:

    // INTERNAL VARIABLES
    Vexample__Syms* vlSymsp;
    const char* vlNamep;

    // CONSTRUCTORS
    Vexample_std__03a__03aprocess__Vclpkg() = default;
    ~Vexample_std__03a__03aprocess__Vclpkg() = default;
    void ctor(Vexample__Syms* symsp, const char* namep);
    void dtor();
    VL_UNCOPYABLE(Vexample_std__03a__03aprocess__Vclpkg);

    // INTERNAL METHODS
    void __Vconfigure(bool first);
    void __VnoInFunc_killQueue(Vexample__Syms* __restrict vlSymsp, VlQueue<VlClassRef<Vexample_std__03a__03aprocess>> &processQueue);
    void __VnoInFunc_self(VlProcessRef vlProcess, Vexample__Syms* __restrict vlSymsp, VlClassRef<Vexample_std__03a__03aprocess> &self__Vfuncrtn);
};


class Vexample__Syms;

class Vexample_std__03a__03aprocess : public virtual VlClass {
  public:

    // DESIGN SPECIFIC STATE
    CData/*0:0*/ __Vtrigprevexpr_h4b30777f__0;
    VlProcessRef __PVT__m_process;
    VlCoroutine __VnoInFunc_await(Vexample__Syms* __restrict vlSymsp);
    void __VnoInFunc_get_randstate(Vexample__Syms* __restrict vlSymsp, std::string &get_randstate__Vfuncrtn);
    void __VnoInFunc_kill(Vexample__Syms* __restrict vlSymsp);
    void __VnoInFunc_resume(Vexample__Syms* __restrict vlSymsp);
    void __VnoInFunc_set_randstate(Vexample__Syms* __restrict vlSymsp, std::string s);
    void __VnoInFunc_set_status(Vexample__Syms* __restrict vlSymsp, IData/*31:0*/ s);
    void __VnoInFunc_status(Vexample__Syms* __restrict vlSymsp, IData/*31:0*/ &status__Vfuncrtn);
    void __VnoInFunc_suspend(Vexample__Syms* __restrict vlSymsp);
  private:
    void _ctor_var_reset(Vexample__Syms* __restrict vlSymsp);
  public:
    Vexample_std__03a__03aprocess() = default;
    void init(Vexample__Syms* __restrict vlSymsp);
    std::string to_string() const;
    std::string to_string_middle() const;
    ~Vexample_std__03a__03aprocess() {}
};

std::string VL_TO_STRING(const VlClassRef<Vexample_std__03a__03aprocess>& obj);


//*** Below code from `systemc in Verilog file
// From `systemc at /home/ant/verilator-infrastructure-verilator4/include/verilated_std.sv:196:21

template<> template<>
inline bool VlClassRef<Vexample_std__03a__03aprocess>::operator==(const VlClassRef<Vexample_std__03a__03aprocess>& rhs) const {
    if (!m_objp && !rhs.m_objp) return true;
    if (!m_objp || !rhs.m_objp) return false;
    return m_objp->__PVT__m_process == rhs.m_objp->__PVT__m_process;
};
template<> template<>
inline bool VlClassRef<Vexample_std__03a__03aprocess>::operator!=(const VlClassRef<Vexample_std__03a__03aprocess>& rhs) const {
    if (!m_objp && !rhs.m_objp) return false;
    if (!m_objp || !rhs.m_objp) return true;
    return m_objp->__PVT__m_process != rhs.m_objp->__PVT__m_process;
};
template<> template<>
inline bool VlClassRef<Vexample_std__03a__03aprocess>::operator<(const VlClassRef<Vexample_std__03a__03aprocess>& rhs) const {
    if (!m_objp && !rhs.m_objp) return false;
    if (!m_objp || !rhs.m_objp) return false;
    return m_objp->__PVT__m_process < rhs.m_objp->__PVT__m_process;
};
//*** Above code from `systemc in Verilog file


#endif  // guard
