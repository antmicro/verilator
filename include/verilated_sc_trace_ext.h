// -*- mode: C++; c-file-style: "cc-mode" -*-
//=============================================================================
//
// Copyright 2001-2025 by Wilson Snyder. This program is free software; you can
// redistribute it and/or modify it under the terms of either the GNU
// Lesser General Public License Version 3 or the Perl Artistic License
// Version 2.0.
// SPDX-License-Identifier: LGPL-3.0-only OR Artistic-2.0
//
//=============================================================================
///
/// \file
/// \brief Verilated tracing of external SystemC signals
///
/// User wrapper code should use this header when tracing external SystemC signals
///
///
//=============================================================================

#ifndef VERILATOR_VERILATED_SC_TRACE_EXT_H_
#define VERILATOR_VERILATED_SC_TRACE_EXT_H_

#include "verilatedos.h"

#include "verilated.h"
#include "verilated_trace.h"

template <typename T_ScSignal, typename T_VerilatedIfs>
struct ScSignalExternalVariable
    : public VerilatedTraceBufferExternalVariable<typename T_VerilatedIfs::Buffer> {
    explicit ScSignalExternalVariable(const T_ScSignal& sig, T_VerilatedIfs& ifp)
        : m_sig{sig}
        , m_ifp{ifp} {}
    const T_ScSignal& m_sig;
    T_VerilatedIfs& m_ifp;
    using Buffer = typename T_VerilatedIfs::Buffer;

    CData val() const {
        CData out;
        VL_ASSIGN_ISI(bits(), out, m_sig);
        return out;
    }
    void decl(IData offset) override {
        m_ifp.spTrace()->declBit(offset, 0, name().c_str(), -1, VerilatedTraceSigDirection::INPUT,
                                 VerilatedTraceSigKind::VAR, VerilatedTraceSigType::BIT, false,
                                 -1);
    }
    void chg(Buffer* bufferp, uint32_t* oldp) override {
        const CData newval = val();
        bufferp->chgBit(oldp, newval);
    }
    void full(Buffer* bufferp, uint32_t* oldp) override {
        uint32_t code = oldp - m_ifp.spTrace()->sigsOldVal();
        const CData newval = val();
        *oldp = newval;  // Still copy even if not tracing so chg doesn't call full
        if (VL_UNLIKELY(m_ifp.spTrace()->sigsEnabled()
                        && !(VL_BITISSET_W(m_ifp.spTrace()->sigsEnabled(), code))))
            return;
        bufferp->emitBit(code, newval);
    }

    std::string name() const override { return m_sig.name(); }
    int bits() const override { return 1; }
    virtual ~ScSignalExternalVariable() override = default;
};
template <typename T_Signal, typename T_Ifp>
auto makeScSignalExternalVariable(const T_Signal& sig, T_Ifp& tfp) {
    return ScSignalExternalVariable<T_Signal, T_Ifp>(sig, tfp);
}

template <typename T_ScSignal, typename T_VerilatedIfs>
struct ScSignalExternalVariableCData
    : public VerilatedTraceBufferExternalVariable<typename T_VerilatedIfs::Buffer> {
    explicit ScSignalExternalVariableCData(const T_ScSignal& sig, T_VerilatedIfs& ifp)
        : m_sig{sig}
        , m_ifp{ifp} {}
    const T_ScSignal& m_sig;
    T_VerilatedIfs& m_ifp;
    using Buffer = typename T_VerilatedIfs::Buffer;

    void decl(IData offset) override {
        m_ifp.spTrace()->declBus(offset, 0, name().c_str(), -1, VerilatedTraceSigDirection::INPUT,
                                 VerilatedTraceSigKind::VAR, VerilatedTraceSigType::INT, false, -1,
                                 bits() - 1, 0);
    }
    void chg(Buffer* bufferp, uint32_t* oldp) override {
        const CData newval = val();
        bufferp->chgCData(oldp, newval, bits());
    }
    void full(Buffer* bufferp, uint32_t* oldp) override {
        uint32_t code = oldp - m_ifp.spTrace()->sigsOldVal();
        const CData newval = val();
        *oldp = newval;  // Still copy even if not tracing so chg doesn't call full
        if (VL_UNLIKELY(m_ifp.spTrace()->sigsEnabled()
                        && !(VL_BITISSET_W(m_ifp.spTrace()->sigsEnabled(), code))))
            return;
        bufferp->emitCData(code, newval, bits());
    }

    CData val() const {
        CData out;
        VL_ASSIGN_ISI(bits(), out, m_sig);
        return out;
    }

    std::string name() const override { return m_sig.name(); }
    int bits() const override { return 8; }
    virtual ~ScSignalExternalVariableCData() override = default;
};
template <typename T_Signal, typename T_Ifp>
auto makeScSignalExternalVariableCData(const T_Signal& sig, T_Ifp& tfp) {
    return ScSignalExternalVariableCData<T_Signal, T_Ifp>(sig, tfp);
}

#endif  // Guard
