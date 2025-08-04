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
//=============================================================================

#ifndef VERILATOR_VERILATED_SC_TRACE_EXT_H_
#define VERILATOR_VERILATED_SC_TRACE_EXT_H_

#include "verilatedos.h"

#include "verilated.h"
#include "verilated_trace.h"

#include <limits>
#include <systemc>
#include <type_traits>

template <typename T_ScSignal, typename T_VerilatedIf>
class ScSignalExternalVariable final
    : public VerilatedTraceBufferExternalSignal<typename T_VerilatedIf::Buffer> {

    using Buffer = typename T_VerilatedIf::Buffer;
    const T_ScSignal& m_sig;
    T_VerilatedIf* m_ifp;
    const std::string m_name;
    const int m_bits;
    WDataOutP m_val;  // TODO only for WData

public:
    explicit ScSignalExternalVariable(const T_ScSignal& sig, T_VerilatedIf* ifp)
        : m_sig{sig}
        , m_ifp{ifp}
        , m_name{sig.name()}
        , m_bits{ScSignalExposer<T_ScSignal>::length(&sig)}
        , m_val{new WData[VL_WORDS_I(m_bits)]} {}

    int decl(IData offset) override {
        const auto prefixAndNamePair = prefixAndName();
        const int levels = m_ifp->pushPrefixUnrolled(prefixAndNamePair.first,
                                                     VerilatedTracePrefixType::SCOPE_MODULE);

        declImpl(offset, prefixAndNamePair.second);
        for (int i = 0; i < levels; ++i) m_ifp->popPrefix();
        return VL_WORDS_I(bits());
    }
    int chg(Buffer* bufferp, uint32_t* oldp) override {
        chgImpl(bufferp, oldp);
        return VL_WORDS_I(bits());
    }
    int full(Buffer* bufferp, uint32_t* oldp) override {
        uint32_t code = oldp - m_ifp->sigsOldVal();
        fullAssignOldImpl(oldp);
        if (VL_UNLIKELY(m_ifp->sigsEnabled() && !(VL_BITISSET_W(m_ifp->sigsEnabled(), code))))
            return VL_WORDS_I(bits());
        fullEmitImpl(bufferp, code);
        return VL_WORDS_I(bits());
    }

    ~ScSignalExternalVariable() override { delete[] m_val; };

private:
    template <typename T_Sig, typename = void>
    struct ScSignalTypeExposer final {
        static_assert(sizeof(T_Sig) == 0, "Internal error: Unsupported signal type");
    };

    template <typename T_Sig>
    struct ScSignalTypeExposer<T_Sig, typename std::enable_if<std::is_base_of<
                                          typename sc_core::sc_port_base, T_Sig>::value>::type>
        final : private T_Sig {
        using Type = typename T_Sig::data_type;
    };

    template <typename T_Sig>
    struct ScSignalTypeExposer<
        T_Sig, typename std::enable_if<
                   std::is_base_of<typename sc_core::sc_signal_channel, T_Sig>::value>::type>
        final : private T_Sig {
        using Type = typename T_Sig::value_type;
    };

    template <typename T, typename = void>
    struct ScSignalHasLengthSfinae final : std::false_type {};

    template <typename... Ts>
    struct make_void final {
        using type = void;
    };

    template <typename... Ts>
    using void_t = typename make_void<Ts...>::type;

    template <typename T>
    struct ScSignalHasLengthSfinae<T, void_t<decltype(int(std::declval<T&>().read().length()))>>
        final : std::true_type {};

    template <typename T>
    static constexpr bool ScSignalHasLength = ScSignalHasLengthSfinae<T>::value;

    template <typename T_Sig, typename = void>
    struct ScSignalExposer final {
        static_assert(sizeof(T_Sig) == 0, "Could not infer signal length");
    };

    template <typename T_Sig>
    struct ScSignalExposer<T_Sig, typename std::enable_if<!ScSignalHasLength<T_Sig>>::type> final {
        static constexpr int length(const T_Sig* /*unused*/) {
            return std::numeric_limits<typename ScSignalTypeExposer<T_Sig>::Type>::digits;
        }
    };

    template <typename T_Sig>
    struct ScSignalExposer<T_Sig, typename std::enable_if<ScSignalHasLength<T_Sig>>::type> final {
        static int length(const T_Sig* signal) { return signal->read().length(); }
    };

    std::pair<std::string, std::string> prefixAndName() {
        std::string name = m_name;
        const size_t idx = name.rfind(sc_core::SC_HIERARCHY_CHAR);
        // TODO
        assert(idx != std::string::npos);
        std::string prefix = name.substr(0, idx);
        name = name.substr(idx + 1);
        // First non-empty scope is a top module
        // TODO consider if we really need to access internal representation
        for (auto& px : m_ifp->m_prefixStack) {
            std::string scope = px.first;
            if (!scope.empty()) {
                // Trim right
                scope.erase(scope.find_last_not_of(' ') + 1);
                if (prefix.find(scope) == 0) {
                    prefix = prefix.substr(scope.size());
                    if (prefix[0] == sc_core::SC_HIERARCHY_CHAR && prefix.size() > 1)
                        prefix = prefix.substr(1);
                }
                break;
            }
        }
        return {prefix, name};
    }

    template <typename T_Sig, std::size_t T_From, std::size_t T_To, typename T_Returns = void>
    using EnableForScSignalOfWidth =
        typename std::enable_if<std::is_integral<typename ScSignalTypeExposer<T_Sig>::Type>::value
                                    && ScSignalExposer<T_Sig>::length(nullptr) >= T_From
                                    && ScSignalExposer<T_Sig>::length(nullptr) <= T_To,
                                T_Returns>::type;

    template <typename T_Sig, typename T_Returns = void>
    using EnableForScSignalFloating = typename std::enable_if<
        std::is_floating_point<typename ScSignalTypeExposer<T_Sig>::Type>::value, T_Returns>::type;

    template <typename T_Sig, typename T_Base, typename T_Returns = void>
    using EnableForSignalOfBaseType = typename std::enable_if<
        std::is_base_of<T_Base, typename ScSignalTypeExposer<T_Sig>::Type>::value,
        T_Returns>::type;

    template <typename T_Sig = T_ScSignal>
    EnableForScSignalOfWidth<T_Sig, 1, 1> declImpl(IData offset, const std::string& name) {
        // TODO handle different sig directions
        m_ifp->declBit(offset, 0, name.c_str(), -1, VerilatedTraceSigDirection::INPUT,
                       VerilatedTraceSigKind::VAR, VerilatedTraceSigType::BIT, false, -1);
    }
    template <typename T_Sig = T_ScSignal>
    EnableForScSignalOfWidth<T_Sig, 2, VL_IDATASIZE> declImpl(IData offset,
                                                              const std::string& name) {
        m_ifp->declBus(offset, 0, name.c_str(), -1, VerilatedTraceSigDirection::INPUT,
                       VerilatedTraceSigKind::VAR, VerilatedTraceSigType::INT, false, -1,
                       bits() - 1, 0);
    }
    template <typename T_Sig = T_ScSignal>
    EnableForScSignalOfWidth<T_Sig, VL_IDATASIZE + 1, VL_QUADSIZE>
    declImpl(IData offset, const std::string& name) {
        m_ifp->declQuad(offset, 0, name.c_str(), -1, VerilatedTraceSigDirection::INPUT,
                        VerilatedTraceSigKind::VAR, VerilatedTraceSigType::LONGINT, false, -1,
                        bits() - 1, 0);
    }
    template <typename T_Sig = T_ScSignal>
    EnableForScSignalFloating<T_Sig> declImpl(IData offset, const std::string& name) {
        m_ifp->declDouble(offset, 0, name.c_str(), -1, VerilatedTraceSigDirection::INPUT,
                          VerilatedTraceSigKind::VAR, VerilatedTraceSigType::DOUBLE, false, -1);
    }
    template <typename T_Sig = T_ScSignal>
    EnableForSignalOfBaseType<T_Sig, sc_dt::sc_bv_base> declImpl(IData offset,
                                                                 const std::string& name) {
        m_ifp->declArray(offset, 0, name.c_str(), -1, VerilatedTraceSigDirection::INPUT,
                         VerilatedTraceSigKind::VAR, VerilatedTraceSigType::LOGIC, false, -1,
                         bits() - 1, 0);
    }
    template <typename T_Sig = T_ScSignal>
    EnableForSignalOfBaseType<T_Sig, sc_dt::sc_uint_base> declImpl(IData offset,
                                                                   const std::string& name) {
        m_ifp->declQuad(offset, 0, name.c_str(), -1, VerilatedTraceSigDirection::INPUT,
                        VerilatedTraceSigKind::VAR, VerilatedTraceSigType::LONGINT, false, -1,
                        bits() - 1, 0);
    }

    template <typename T_Sig = T_ScSignal>
    EnableForScSignalOfWidth<T_Sig, 1, 1> chgImpl(Buffer* bufferp, uint32_t* oldp) {
        bufferp->chgBit(oldp, val());
    }
    template <typename T_Sig = T_ScSignal>
    EnableForScSignalOfWidth<T_Sig, 2, VL_BYTESIZE> chgImpl(Buffer* bufferp, uint32_t* oldp) {
        bufferp->chgCData(oldp, val(), bits());
    }
    template <typename T_Sig = T_ScSignal>
    EnableForScSignalOfWidth<T_Sig, VL_BYTESIZE + 1, VL_SHORTSIZE> chgImpl(Buffer* bufferp,
                                                                           uint32_t* oldp) {
        bufferp->chgSData(oldp, val(), bits());
    }
    template <typename T_Sig = T_ScSignal>
    EnableForScSignalOfWidth<T_Sig, VL_SHORTSIZE + 1, VL_IDATASIZE> chgImpl(Buffer* bufferp,
                                                                            uint32_t* oldp) {
        bufferp->chgIData(oldp, val(), bits());
    }
    template <typename T_Sig = T_ScSignal>
    EnableForScSignalOfWidth<T_Sig, VL_IDATASIZE + 1, VL_QUADSIZE> chgImpl(Buffer* bufferp,
                                                                           uint32_t* oldp) {
        bufferp->chgQData(oldp, val(), bits());
    }
    template <typename T_Sig = T_ScSignal>
    EnableForScSignalFloating<T_Sig> chgImpl(Buffer* bufferp, uint32_t* oldp) {
        bufferp->chgDouble(oldp, val());
    }
    template <typename T_Sig = T_ScSignal>
    EnableForSignalOfBaseType<T_Sig, sc_dt::sc_bv_base> chgImpl(Buffer* bufferp, uint32_t* oldp) {
        bufferp->chgWData(oldp, val(), bits());
    }
    template <typename T_Sig = T_ScSignal>
    EnableForSignalOfBaseType<T_Sig, sc_dt::sc_uint_base> chgImpl(Buffer* bufferp,
                                                                  uint32_t* oldp) {
        bufferp->chgQData(oldp, val(), bits());
    }

    template <typename T_Sig = T_ScSignal>
    EnableForScSignalOfWidth<T_Sig, 1, VL_IDATASIZE> fullAssignOldImpl(uint32_t* oldp) {
        *oldp = val();  // Still copy even if not tracing so chg doesn't call full
    }
    // TODO Simplify memcpy assigns
    template <typename T_Sig = T_ScSignal>
    EnableForScSignalOfWidth<T_Sig, VL_IDATASIZE + 1, VL_QUADSIZE>
    fullAssignOldImpl(uint32_t* oldp) {
        // Still copy even if not tracing so chg doesn't call full
        const QData newval = val();
        std::memcpy(oldp, &newval, sizeof(newval));
    }
    template <typename T_Sig = T_ScSignal>
    EnableForScSignalFloating<T_Sig> fullAssignOldImpl(uint32_t* oldp) {
        // Still copy even if not tracing so chg doesn't call full
        const double newval = val();
        std::memcpy(oldp, &newval, sizeof(newval));
    }
    template <typename T_Sig = T_ScSignal>
    EnableForSignalOfBaseType<T_Sig, sc_dt::sc_bv_base> fullAssignOldImpl(uint32_t* oldp) {
        // Still copy even if not tracing so chg doesn't call full
        WDataOutP newvalp = val();
        for (int i = 0; i < VL_WORDS_I(bits()); ++i) oldp[i] = newvalp[i];
    }
    // TODO simplify with QData
    template <typename T_Sig = T_ScSignal>
    EnableForSignalOfBaseType<T_Sig, sc_dt::sc_uint_base> fullAssignOldImpl(uint32_t* oldp) {
        // Still copy even if not tracing so chg doesn't call full
        const QData newval = val();
        std::memcpy(oldp, &newval, sizeof(newval));
    }

    template <typename T_Sig = T_ScSignal>
    EnableForScSignalOfWidth<T_Sig, 1, 1> fullEmitImpl(Buffer* bufferp, uint32_t code) {
        VerilatedTraceBufferExternalSignal<Buffer>::emitBit(bufferp, code, val());
    }
    template <typename T_Sig = T_ScSignal>
    EnableForScSignalOfWidth<T_Sig, 2, VL_BYTESIZE> fullEmitImpl(Buffer* bufferp, uint32_t code) {
        VerilatedTraceBufferExternalSignal<Buffer>::emitCData(bufferp, code, val(), bits());
    }
    template <typename T_Sig = T_ScSignal>
    EnableForScSignalOfWidth<T_Sig, VL_BYTESIZE + 1, VL_SHORTSIZE> fullEmitImpl(Buffer* bufferp,
                                                                                uint32_t code) {
        VerilatedTraceBufferExternalSignal<Buffer>::emitSData(bufferp, code, val(), bits());
    }
    template <typename T_Sig = T_ScSignal>
    EnableForScSignalOfWidth<T_Sig, VL_SHORTSIZE + 1, VL_IDATASIZE> fullEmitImpl(Buffer* bufferp,
                                                                                 uint32_t code) {
        VerilatedTraceBufferExternalSignal<Buffer>::emitIData(bufferp, code, val(), bits());
    }
    template <typename T_Sig = T_ScSignal>
    EnableForScSignalOfWidth<T_Sig, VL_IDATASIZE + 1, VL_QUADSIZE> fullEmitImpl(Buffer* bufferp,
                                                                                uint32_t code) {
        VerilatedTraceBufferExternalSignal<Buffer>::emitQData(bufferp, code, val(), bits());
    }
    template <typename T_Sig = T_ScSignal>
    EnableForScSignalFloating<T_Sig> fullEmitImpl(Buffer* bufferp, uint32_t code) {
        VerilatedTraceBufferExternalSignal<Buffer>::emitDouble(bufferp, code, val());
    }
    template <typename T_Sig = T_ScSignal>
    EnableForSignalOfBaseType<T_Sig, sc_dt::sc_bv_base> fullEmitImpl(Buffer* bufferp,
                                                                     uint32_t code) {
        VerilatedTraceBufferExternalSignal<Buffer>::emitWData(bufferp, code, val(), bits());
    }
    template <typename T_Sig = T_ScSignal>
    EnableForSignalOfBaseType<T_Sig, sc_dt::sc_uint_base> fullEmitImpl(Buffer* bufferp,
                                                                       uint32_t code) {
        VerilatedTraceBufferExternalSignal<Buffer>::emitQData(bufferp, code, val(), bits());
    }

    int bits() const { return m_bits; }

    template <typename T_Sig = T_ScSignal>
    EnableForScSignalOfWidth<T_Sig, 1, VL_IDATASIZE, IData> val() const {
        return VL_CLEAN_II(bits(), bits(), m_sig.read());
    }
    template <typename T_Sig = T_ScSignal>
    EnableForScSignalOfWidth<T_Sig, VL_IDATASIZE + 1, VL_QUADSIZE, QData> val() const {
        return VL_CLEAN_QQ(bits(), bits(), m_sig.read());
    }
    template <typename T_Sig = T_ScSignal>
    EnableForScSignalFloating<T_Sig, double> val() const {
        double result{};
        VL_ASSIGN_DSD(bits(), result, m_sig);
        return result;
    }
    template <typename T_Sig = T_ScSignal>
    EnableForSignalOfBaseType<T_Sig, sc_dt::sc_bv_base, WDataOutP> val() const {
        VL_ASSIGN_WSW(bits(), m_val, m_sig);
        return m_val;
    }
    template <typename T_Sig = T_ScSignal>
    EnableForSignalOfBaseType<T_Sig, sc_dt::sc_uint_base, QData> val() const {
        QData out{};
        VL_ASSIGN_QSU(bits(), out, m_sig);
        return out;
    }
};

#endif  // Guard
