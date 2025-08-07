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

template <typename T_ScSignal, typename = void>
struct ScSignalTypeExposer;

template <typename T_ScSignal>
struct ScSignalTypeExposer<
    T_ScSignal, typename std::enable_if<
                    std::is_base_of<typename sc_core::sc_port_base, T_ScSignal>::value>::type>
    final : private T_ScSignal {
    using Type = typename T_ScSignal::data_type;
};

template <typename T_ScSignal>
struct ScSignalTypeExposer<
    T_ScSignal, typename std::enable_if<
                    std::is_base_of<typename sc_core::sc_signal_channel, T_ScSignal>::value>::type>
    final : private T_ScSignal {
    using Type = typename T_ScSignal::value_type;
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
struct ScSignalHasLengthSfinae<T, void_t<decltype(int(std::declval<T&>().read().length()))>> final
    : std::true_type {};

template <typename T>
constexpr bool ScSignalHasLength = ScSignalHasLengthSfinae<T>::value;

template <typename T_ScSignal, typename = void>
struct ScSignalExposer;

template <typename T_ScSignal>
struct ScSignalExposer<T_ScSignal, typename std::enable_if<!ScSignalHasLength<T_ScSignal>>::type>
    final {
    static constexpr int length() {
        return std::numeric_limits<typename ScSignalTypeExposer<T_ScSignal>::Type>::digits;
    }
};

template <typename T_ScSignal>
struct ScSignalExposer<T_ScSignal, typename std::enable_if<ScSignalHasLength<T_ScSignal>>::type>
    final {
    static int length(const T_ScSignal& signal) { return signal.read().length(); }
};

template <typename T_ScSignal, std::size_t T_From, std::size_t T_To>
using EnableForScSignalOfWidth =
    typename std::enable_if<std::is_integral<typename ScSignalTypeExposer<T_ScSignal>::Type>::value
                            && ScSignalExposer<T_ScSignal>::length() >= T_From
                            && ScSignalExposer<T_ScSignal>::length() <= T_To>::type;

template <typename T_ScSignal, typename T_Base>
using EnableForSignalOfBaseType = typename std::enable_if<
    std::is_base_of<T_Base, typename ScSignalTypeExposer<T_ScSignal>::Type>::value>::type;

template <typename T_ScSignal, typename T_VerilatedIf, typename = void>
class ScSignalExternalVariable;

template <typename T_ScSignal, typename T_VerilatedIf>
class ScSignalExternalVariable<T_ScSignal, T_VerilatedIf,
                               EnableForScSignalOfWidth<T_ScSignal, 1, 1>>
    final : public VerilatedTraceBufferExternalSignal<typename T_VerilatedIf::Buffer> {

    using Buffer = typename T_VerilatedIf::Buffer;

    const T_ScSignal& m_sig;
    T_VerilatedIf* m_ifp;
    const std::string m_name;

public:
    explicit ScSignalExternalVariable(const T_ScSignal& sig, T_VerilatedIf* ifp)
        : m_sig{sig}
        , m_ifp{ifp}
        , m_name{sig.name()} {}
    int decl(IData offset) override {
        m_ifp->declBit(offset, 0, m_name.c_str(), -1, VerilatedTraceSigDirection::INPUT,
                       VerilatedTraceSigKind::VAR, VerilatedTraceSigType::BIT, false, -1);
        return VL_WORDS_I(bits());
    }
    int chg(Buffer* bufferp, uint32_t* oldp) override {
        bufferp->chgBit(oldp, val());
        return VL_WORDS_I(bits());
    }
    int full(Buffer* bufferp, uint32_t* oldp) override {
        uint32_t code = oldp - m_ifp->sigsOldVal();
        *oldp = val();  // Still copy even if not tracing so chg doesn't call full
        if (VL_UNLIKELY(m_ifp->sigsEnabled() && !(VL_BITISSET_W(m_ifp->sigsEnabled(), code))))
            return VL_WORDS_I(bits());
        VerilatedTraceBufferExternalSignal<Buffer>::emitBit(bufferp, code, *oldp);
        return VL_WORDS_I(bits());
    }

    ~ScSignalExternalVariable() override = default;

private:
    static constexpr int bits() { return ScSignalExposer<T_ScSignal>::length(); }

    IData val() const { return VL_CLEAN_II(bits(), bits(), m_sig.read()); }
};

template <typename T_ScSignal, typename T_VerilatedIf>
class ScSignalExternalVariable<T_ScSignal, T_VerilatedIf,
                               EnableForScSignalOfWidth<T_ScSignal, 2, VL_BYTESIZE>>
    final : public VerilatedTraceBufferExternalSignal<typename T_VerilatedIf::Buffer> {

    using Buffer = typename T_VerilatedIf::Buffer;

    const T_ScSignal& m_sig;
    T_VerilatedIf* m_ifp;
    const std::string m_name;

public:
    explicit ScSignalExternalVariable(const T_ScSignal& sig, T_VerilatedIf* ifp)
        : m_sig{sig}
        , m_ifp{ifp}
        , m_name{sig.name()} {}
    int decl(IData offset) override {
        m_ifp->declBus(offset, 0, m_name.c_str(), -1, VerilatedTraceSigDirection::INPUT,
                       VerilatedTraceSigKind::VAR, VerilatedTraceSigType::INT, false, -1,
                       bits() - 1, 0);
        return VL_WORDS_I(bits());
    }
    int chg(Buffer* bufferp, uint32_t* oldp) override {
        bufferp->chgCData(oldp, val(), bits());
        return VL_WORDS_I(bits());
    }
    int full(Buffer* bufferp, uint32_t* oldp) override {
        uint32_t code = oldp - m_ifp->sigsOldVal();
        *oldp = val();  // Still copy even if not tracing so chg doesn't call full
        if (VL_UNLIKELY(m_ifp->sigsEnabled() && !(VL_BITISSET_W(m_ifp->sigsEnabled(), code))))
            return VL_WORDS_I(bits());
        VerilatedTraceBufferExternalSignal<Buffer>::emitCData(bufferp, code, *oldp, bits());
        return VL_WORDS_I(bits());
    }

    ~ScSignalExternalVariable() override = default;

private:
    static constexpr int bits() { return ScSignalExposer<T_ScSignal>::length(); }

    IData val() const { return VL_CLEAN_II(bits(), bits(), m_sig.read()); }
};

template <typename T_ScSignal, typename T_VerilatedIf>
class ScSignalExternalVariable<T_ScSignal, T_VerilatedIf,
                               EnableForScSignalOfWidth<T_ScSignal, VL_BYTESIZE + 1, VL_SHORTSIZE>>
    final : public VerilatedTraceBufferExternalSignal<typename T_VerilatedIf::Buffer> {

    using Buffer = typename T_VerilatedIf::Buffer;

    const T_ScSignal& m_sig;
    T_VerilatedIf* m_ifp;
    const std::string m_name;

public:
    explicit ScSignalExternalVariable(const T_ScSignal& sig, T_VerilatedIf* ifp)
        : m_sig{sig}
        , m_ifp{ifp}
        , m_name{sig.name()} {}
    int decl(IData offset) override {
        m_ifp->declBus(offset, 0, m_name.c_str(), -1, VerilatedTraceSigDirection::INPUT,
                       VerilatedTraceSigKind::VAR, VerilatedTraceSigType::INT, false, -1,
                       bits() - 1, 0);
        return VL_WORDS_I(bits());
    }
    int chg(Buffer* bufferp, uint32_t* oldp) override {
        bufferp->chgSData(oldp, val(), bits());
        return VL_WORDS_I(bits());
    }
    int full(Buffer* bufferp, uint32_t* oldp) override {
        uint32_t code = oldp - m_ifp->sigsOldVal();
        *oldp = val();  // Still copy even if not tracing so chg doesn't call full
        if (VL_UNLIKELY(m_ifp->sigsEnabled() && !(VL_BITISSET_W(m_ifp->sigsEnabled(), code))))
            return VL_WORDS_I(bits());
        VerilatedTraceBufferExternalSignal<Buffer>::emitSData(bufferp, code, *oldp, bits());
        return VL_WORDS_I(bits());
    }

    ~ScSignalExternalVariable() override = default;

private:
    static constexpr int bits() { return ScSignalExposer<T_ScSignal>::length(); }

    IData val() const { return VL_CLEAN_II(bits(), bits(), m_sig.read()); }
};

template <typename T_ScSignal, typename T_VerilatedIf>
class ScSignalExternalVariable<
    T_ScSignal, T_VerilatedIf,
    EnableForScSignalOfWidth<T_ScSignal, VL_SHORTSIZE + 1, VL_IDATASIZE>>
    final : public VerilatedTraceBufferExternalSignal<typename T_VerilatedIf::Buffer> {

    using Buffer = typename T_VerilatedIf::Buffer;

    const T_ScSignal& m_sig;
    T_VerilatedIf* m_ifp;
    const std::string m_name;

public:
    explicit ScSignalExternalVariable(const T_ScSignal& sig, T_VerilatedIf* ifp)
        : m_sig{sig}
        , m_ifp{ifp}
        , m_name{sig.name()} {}
    int decl(IData offset) override {
        m_ifp->declBus(offset, 0, m_name.c_str(), -1, VerilatedTraceSigDirection::INPUT,
                       VerilatedTraceSigKind::VAR, VerilatedTraceSigType::INT, false, -1,
                       bits() - 1, 0);
        return VL_WORDS_I(bits());
    }
    int chg(Buffer* bufferp, uint32_t* oldp) override {
        bufferp->chgIData(oldp, val(), bits());

        return VL_WORDS_I(bits());
    }
    int full(Buffer* bufferp, uint32_t* oldp) override {
        uint32_t code = oldp - m_ifp->sigsOldVal();
        *oldp = val();  // Still copy even if not tracing so chg doesn't call full
        if (VL_UNLIKELY(m_ifp->sigsEnabled() && !(VL_BITISSET_W(m_ifp->sigsEnabled(), code))))
            return VL_WORDS_I(bits());
        VerilatedTraceBufferExternalSignal<Buffer>::emitIData(bufferp, code, *oldp, bits());
        return VL_WORDS_I(bits());
    }

    ~ScSignalExternalVariable() override = default;

private:
    static constexpr int bits() { return ScSignalExposer<T_ScSignal>::length(); }

    IData val() const { return VL_CLEAN_II(bits(), bits(), m_sig.read()); }
};

template <typename T_ScSignal, typename T_VerilatedIf>
class ScSignalExternalVariable<T_ScSignal, T_VerilatedIf,
                               EnableForScSignalOfWidth<T_ScSignal, VL_IDATASIZE + 1, VL_QUADSIZE>>
    final : public VerilatedTraceBufferExternalSignal<typename T_VerilatedIf::Buffer> {

    using Buffer = typename T_VerilatedIf::Buffer;

    const T_ScSignal& m_sig;
    T_VerilatedIf* m_ifp;
    const std::string m_name;

public:
    explicit ScSignalExternalVariable(const T_ScSignal& sig, T_VerilatedIf* ifp)
        : m_sig{sig}
        , m_ifp{ifp}
        , m_name{sig.name()} {}
    int decl(IData offset) override {
        m_ifp->declQuad(offset, 0, m_name.c_str(), -1, VerilatedTraceSigDirection::INPUT,
                        VerilatedTraceSigKind::VAR, VerilatedTraceSigType::LONGINT, false, -1,
                        bits() - 1, 0);
        return VL_WORDS_I(bits());
    }
    int chg(Buffer* bufferp, uint32_t* oldp) override {
        bufferp->chgQData(oldp, val(), bits());
        return VL_WORDS_I(bits());
    }
    int full(Buffer* bufferp, uint32_t* oldp) override {
        const uint32_t code = oldp - m_ifp->sigsOldVal();
        const QData newval = val();
        std::memcpy(oldp, &newval, sizeof(newval));
        if (VL_UNLIKELY(m_ifp->sigsEnabled() && !(VL_BITISSET_W(m_ifp->sigsEnabled(), code))))
            return VL_WORDS_I(bits());
        VerilatedTraceBufferExternalSignal<Buffer>::emitQData(bufferp, code, newval, bits());
        return VL_WORDS_I(bits());
    }

    ~ScSignalExternalVariable() override = default;

private:
    static constexpr int bits() { return ScSignalExposer<T_ScSignal>::length(); }

    QData val() const { return VL_CLEAN_QQ(bits(), bits(), m_sig.read()); }
};

template <typename T_ScSignal, typename T_VerilatedIf>
class ScSignalExternalVariable<T_ScSignal, T_VerilatedIf,
                               typename std::enable_if<std::is_floating_point<
                                   typename ScSignalTypeExposer<T_ScSignal>::Type>::value>::type>
    final : public VerilatedTraceBufferExternalSignal<typename T_VerilatedIf::Buffer> {

    using Buffer = typename T_VerilatedIf::Buffer;

    const T_ScSignal& m_sig;
    T_VerilatedIf* m_ifp;
    const std::string m_name;

public:
    explicit ScSignalExternalVariable(const T_ScSignal& sig, T_VerilatedIf* ifp)
        : m_sig{sig}
        , m_ifp{ifp}
        , m_name{sig.name()} {}

    int decl(IData offset) override {
        m_ifp->declDouble(offset, 0, m_name.c_str(), -1, VerilatedTraceSigDirection::INPUT,
                          VerilatedTraceSigKind::VAR, VerilatedTraceSigType::DOUBLE, false, -1);
        return VL_WORDS_I(bits());
    }
    int chg(Buffer* bufferp, uint32_t* oldp) override {
        bufferp->chgDouble(oldp, val());
        return VL_WORDS_I(bits());
    }
    int full(Buffer* bufferp, uint32_t* oldp) override {
        const uint32_t code = oldp - m_ifp->sigsOldVal();
        const double newval = val();
        std::memcpy(oldp, &newval, sizeof(newval));
        if (VL_UNLIKELY(m_ifp->sigsEnabled() && !(VL_BITISSET_W(m_ifp->sigsEnabled(), code))))
            return VL_WORDS_I(bits());
        VerilatedTraceBufferExternalSignal<Buffer>::emitDouble(bufferp, code, newval);
        return VL_WORDS_I(bits());
    }

    ~ScSignalExternalVariable() override = default;

private:
    static constexpr int bits() { return ScSignalExposer<T_ScSignal>::length(); }

    double val() const {
        double result{};
        VL_ASSIGN_DSD(bits(), result, m_sig);
        return result;
    }
};

template <typename T_ScSignal, typename T_VerilatedIf>
class ScSignalExternalVariable<T_ScSignal, T_VerilatedIf,
                               EnableForSignalOfBaseType<T_ScSignal, sc_dt::sc_bv_base>>
    final : public VerilatedTraceBufferExternalSignal<typename T_VerilatedIf::Buffer> {

    using Buffer = typename T_VerilatedIf::Buffer;

    const T_ScSignal& m_sig;
    T_VerilatedIf* m_ifp;
    const std::string m_name;
    const int m_bits;
    WDataOutP m_val;

public:
    explicit ScSignalExternalVariable(const T_ScSignal& sig, T_VerilatedIf* ifp)
        : m_sig{sig}
        , m_ifp{ifp}
        , m_name{sig.name()}
        , m_bits{ScSignalExposer<T_ScSignal>::length(sig)}
        , m_val{new WData[VL_WORDS_I(m_bits)]} {}
    int decl(IData offset) override {
        m_ifp->declArray(offset, 0, m_name.c_str(), -1, VerilatedTraceSigDirection::INPUT,
                         VerilatedTraceSigKind::VAR, VerilatedTraceSigType::LOGIC, false, -1,
                         m_bits - 1, 0);
        return VL_WORDS_I(m_bits);
    }
    int chg(Buffer* bufferp, uint32_t* oldp) override {
        bufferp->chgWData(oldp, val(), m_bits);
        return VL_WORDS_I(m_bits);
    }
    int full(Buffer* bufferp, uint32_t* oldp) override {
        const uint32_t code = oldp - m_ifp->sigsOldVal();
        WDataOutP newvalp = val();
        for (int i = 0; i < VL_WORDS_I(m_bits); ++i) oldp[i] = newvalp[i];
        if (VL_UNLIKELY(m_ifp->sigsEnabled() && !(VL_BITISSET_W(m_ifp->sigsEnabled(), code))))
            return VL_WORDS_I(m_bits);
        VerilatedTraceBufferExternalSignal<Buffer>::emitWData(bufferp, code, newvalp, m_bits);
        return VL_WORDS_I(m_bits);
    }

    ~ScSignalExternalVariable() override { delete[] m_val; };

private:
    WDataOutP val() {
        VL_ASSIGN_WSW(m_bits, m_val, m_sig);
        return m_val;
    }
};

template <typename T_ScSignal, typename T_VerilatedIf>
class ScSignalExternalVariable<T_ScSignal, T_VerilatedIf,
                               // TODO distinguish IData and QData case for sc_uint
                               EnableForSignalOfBaseType<T_ScSignal, sc_dt::sc_uint_base>>
    : public VerilatedTraceBufferExternalSignal<typename T_VerilatedIf::Buffer> {

    using Buffer = typename T_VerilatedIf::Buffer;

    const T_ScSignal& m_sig;
    T_VerilatedIf* m_ifp;
    const std::string m_name;
    const int m_bits;

public:
    explicit ScSignalExternalVariable(const T_ScSignal& sig, T_VerilatedIf* ifp)
        : m_sig{sig}
        , m_ifp{ifp}
        , m_name{sig.name()}
        , m_bits{ScSignalExposer<T_ScSignal>::length(sig)} {}
    int decl(IData offset) override {
        m_ifp->declQuad(offset, 0, m_name.c_str(), -1, VerilatedTraceSigDirection::INPUT,
                        VerilatedTraceSigKind::VAR, VerilatedTraceSigType::LONGINT, false, -1,
                        m_bits - 1, 0);
        return VL_WORDS_I(m_bits);
    }
    int chg(Buffer* bufferp, uint32_t* oldp) override {
        bufferp->chgQData(oldp, val(), m_bits);
        return VL_WORDS_I(m_bits);
    }
    int full(Buffer* bufferp, uint32_t* oldp) override {
        const uint32_t code = oldp - m_ifp->sigsOldVal();
        const QData newval = val();
        std::memcpy(oldp, &newval, sizeof(newval));
        if (VL_UNLIKELY(m_ifp->sigsEnabled() && !(VL_BITISSET_W(m_ifp->sigsEnabled(), code))))
            return VL_WORDS_I(m_bits);
        VerilatedTraceBufferExternalSignal<Buffer>::emitQData(bufferp, code, newval, m_bits);
        return VL_WORDS_I(m_bits);
    }

    ~ScSignalExternalVariable() override = default;

private:
    QData val() const {
        QData out{};
        VL_ASSIGN_QSU(m_bits, out, m_sig);
        return out;
    }
};

// Trigger assertions when no specializations were matched.
template <typename T_ScSignal, typename T_VerilatedIf, typename>
class ScSignalExternalVariable final {
    static_assert(sizeof(T_ScSignal) == 0, "Unsupported external variable type");
};

template <typename T_ScSignal, typename>
struct ScSignalTypeExposer final {
    static_assert(sizeof(T_ScSignal) == 0,
                  "Unsupported tracing variables that are not sc_signal_channel");
};

#endif  // Guard
