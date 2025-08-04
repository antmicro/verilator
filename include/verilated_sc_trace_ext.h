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

#include <limits>

template <typename T_ScSignal>
struct ScSignalExposer final : private T_ScSignal {
    using Type = typename T_ScSignal::value_type;
    static constexpr int digits() {
        return std::numeric_limits<typename ScSignalExposer<T_ScSignal>::Type>::digits;
    }
};

template <typename T_ScSignal, typename T_VerilatedIfs, typename Enable = void>
class ScSignalExternalVariable;

template <typename T_ScSignal, typename T_VerilatedIfs>
class ScSignalExternalVariable<
    T_ScSignal, T_VerilatedIfs,
    typename std::enable_if<ScSignalExposer<T_ScSignal>::digits() == 1>::type>
    final : public VerilatedTraceBufferExternalVariable<typename T_VerilatedIfs::Buffer> {
    const T_ScSignal& m_sig;
    T_VerilatedIfs& m_ifp;
    const std::string m_name;
    using Buffer = typename T_VerilatedIfs::Buffer;

    static constexpr int bits() { return ScSignalExposer<T_ScSignal>::digits(); }

public:
    explicit ScSignalExternalVariable(const T_ScSignal& sig, T_VerilatedIfs& ifp)
        : m_sig{sig}
        , m_ifp{ifp}
        , m_name{sig.name()} {}
    void decl(IData offset) override {
        m_ifp.spTrace()->declBit(offset, 0, m_name.c_str(), -1, VerilatedTraceSigDirection::INPUT,
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

    ~ScSignalExternalVariable() override = default;

private:
    CData val() const {
        CData out;
        VL_ASSIGN_ISI(bits(), out, m_sig);
        return out;
    }
};

template <typename T_ScSignal, typename T_VerilatedIfs>
class ScSignalExternalVariable<
    T_ScSignal, T_VerilatedIfs,
    typename std::enable_if<ScSignalExposer<T_ScSignal>::digits() >= 2
                            && ScSignalExposer<T_ScSignal>::digits() <= 8>::type>
    final : public VerilatedTraceBufferExternalVariable<typename T_VerilatedIfs::Buffer> {
    const T_ScSignal& m_sig;
    T_VerilatedIfs& m_ifp;
    const std::string m_name;
    using Buffer = typename T_VerilatedIfs::Buffer;

    static constexpr int bits() { return ScSignalExposer<T_ScSignal>::digits(); }

public:
    explicit ScSignalExternalVariable(const T_ScSignal& sig, T_VerilatedIfs& ifp)
        : m_sig{sig}
        , m_ifp{ifp}
        , m_name{sig.name()} {}
    void decl(IData offset) override {
        m_ifp.spTrace()->declBus(offset, 0, m_name.c_str(), -1, VerilatedTraceSigDirection::INPUT,
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

    ~ScSignalExternalVariable() override = default;

private:
    CData val() const {
        CData out;
        VL_ASSIGN_ISI(bits(), out, m_sig);
        return out;
    }
};

template <typename T_Signal, typename T_Ifp>
auto makeScSignalExternalVariable(const T_Signal& sig, T_Ifp& tfp) {
    return std::make_unique<ScSignalExternalVariable<T_Signal, T_Ifp>>(sig, tfp);
}

#endif  // Guard
