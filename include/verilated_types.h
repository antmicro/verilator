// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
//
// Code available from: https://verilator.org
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of either the GNU Lesser General Public License Version 3
// or the Perl Artistic License Version 2.0.
// SPDX-FileCopyrightText: 2003-2026 Wilson Snyder
// SPDX-License-Identifier: LGPL-3.0-only OR Artistic-2.0
//
//*************************************************************************
///
/// \file
/// \brief Verilated common data type containers
///
/// verilated.h should be included instead of this file.
///
/// Those macro/function/variable starting or ending in _ are internal,
/// however many of the other function/macros here are also internal.
///
//*************************************************************************

#ifndef VERILATOR_VERILATED_TYPES_H_
#define VERILATOR_VERILATED_TYPES_H_

#ifndef VERILATOR_VERILATED_H_INTERNAL_
#error "verilated_types.h should only be included by verilated.h"
#endif

#include <algorithm>
#include <array>
#include <atomic>
#include <deque>
#include <initializer_list>
#include <iterator>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <utility>

class VlClass;
template <typename T_Class>
class VlClassRef;
class VlProcess;
template <typename T_Value, std::size_t N_Depth>
class VlUnpacked;

//=========================================================================
// Debug functions

#ifdef VL_DEBUG
/// Evaluate statement if VL_DEBUG defined
#define VL_DEBUG_IFDEF(stmt) \
    do { stmt } while (false)
/// Evaluate statement if VL_DEBUG defined and Verilated::debug() enabled
#define VL_DEBUG_IF(stmt) \
    do { \
        if (VL_UNLIKELY(Verilated::debug())) { stmt } \
    } while (false)
#else
// We intentionally do not compile the stmt to improve compile speed
#define VL_DEBUG_IFDEF(stmt) \
    do { \
    } while (false)
#define VL_DEBUG_IF(stmt) \
    do { \
    } while (false)
#endif

//===================================================================
/// Verilog wide packed bit container.
/// Similar to std::array<EData, N>, but lighter weight, only methods needed
/// by Verilator, to help compile time.
///
/// A 'struct' as we want this to be an aggregate type that allows
/// static aggregate initialization. Consider data members private.
///
/// For example a Verilog "bit [94:0]" will become a VlWide<3> because 3*32
/// bits are needed to hold the 95 bits. The MSB (bit 96) must always be
/// zero in memory, but during intermediate operations in the Verilated
/// internals is unpredictable.

template <std::size_t N_Words>
struct VlWide final {
    static constexpr size_t Words = N_Words;

    // MEMBERS
    // This should be the only data member, otherwise generated static initializers need updating
    EData m_storage[N_Words];  // Contents of the packed array

    // CONSTRUCTORS
    // Default constructors and destructor are used. Note however that C++20 requires that
    // aggregate types do not have a user declared constructor, not even an explicitly defaulted
    // one.

    // OPERATOR METHODS
    bool operator==(const VlWide<N_Words>& that) const VL_PURE {
        return std::memcmp(m_storage, that.m_storage, N_Words * sizeof(EData)) == 0;
    }
    bool operator!=(const VlWide<N_Words>& that) const VL_PURE { return !(*this == that); }
    EData& operator[](size_t index) VL_MT_SAFE { return m_storage[index]; }
    const EData& operator[](size_t index) const VL_MT_SAFE { return m_storage[index]; }
    VlWide<N_Words>& operator&=(const VlWide& rhs) {
        VL_AND_W(N_Words, *this, *this, rhs);
        return *this;
    }
    VlWide<N_Words>& operator|=(const VlWide& rhs) {
        VL_OR_W(N_Words, *this, *this, rhs);
        return *this;
    }
    VlWide<N_Words>& operator^=(const VlWide& rhs) {
        VL_XOR_W(N_Words, *this, *this, rhs);
        return *this;
    }
    VlWide<N_Words>& operator+=(const VlWide& rhs) {
        VL_ADD_W(N_Words, *this, *this, rhs);
        return *this;
    }
    VlWide<N_Words>& operator*=(const VlWide& rhs) {
        VlWide<N_Words> out{};
        VL_MUL_W(N_Words, out, *this, rhs);
        for (size_t i = 0; i < N_Words; ++i) m_storage[i] = out.m_storage[i];
        return *this;
    }

    // METHODS
    EData& at(size_t index) VL_MT_SAFE { return m_storage[index]; }
    const EData& at(size_t index) const VL_MT_SAFE { return m_storage[index]; }
    size_t size() const VL_PURE { return N_Words; }
    EData* data() VL_MT_SAFE { return &m_storage[0]; }
    const EData* data() const VL_MT_SAFE { return &m_storage[0]; }
    inline bool operator<(const VlWide<N_Words>& rhs) const VL_PURE;
};

// Type trait to check if a type is VlWide
template <typename>
struct VlIsVlWide : public std::false_type {};
template <std::size_t N_Words>
struct VlIsVlWide<VlWide<N_Words>> : public std::true_type {};

// Opaque handles to backing store of a VlWide, These are used to pass the
// backing store to runtime functions without having to template all the
// runtime functions on the VlWide template patameters. They created via
// implicit constructors from any VlWide, so a VlWide can be passed directly
// to functions that take one of these handles. The handles are 'opaque' in
// the sense that they are not primitive C++ types. They have necessary
// operators defined to allow them to behave like pointers to the extent
// it is necessary to implement the runtime functions. They are still only
// a single pointer underneath, so they will be passed in registers.

// Read-Write VlWide handle
class WDataOutP final {
    EData* m_datap;  // The backing store - non const as the handle is assignable

    // Use WDataOutP::external() to create a handle from a raw pointer
    explicit WDataOutP(EData* datap) VL_PURE : m_datap{datap} {}

public:
    // FACTORY METHODS
    // Create a handle from a raw pointer - only to be used in verilated*.h
    static WDataOutP external(EData* datap) VL_PURE { return WDataOutP{datap}; }

    // CONSTRUCTORS
    // Implicit conversion from 'VlWide'
    template <std::size_t N_Words>
    // cppcheck-suppress noExplicitConstructor
    /* implicit */ WDataOutP(VlWide<N_Words>& vlWide) VL_PURE : m_datap{vlWide.data()} {}
    WDataOutP(const WDataOutP& other) VL_PURE = default;
    WDataOutP(WDataOutP&& other) VL_PURE = default;
    WDataOutP& operator=(const WDataOutP& other) VL_PURE = default;

    // METHODS
    EData* datap() const VL_PURE { return m_datap; }
    operator bool() const VL_PURE { return m_datap; }
    EData& operator[](size_t index) const VL_PURE { return m_datap[index]; }
    WDataOutP operator+(size_t index) const VL_PURE { return WDataOutP(m_datap + index); }
    WDataOutP operator+(int index) const VL_PURE { return WDataOutP(m_datap + index); }
};
static_assert(sizeof(WDataOutP) == sizeof(EData*), "WDataOutP should be a single pointer");

// Read-Only VlWide handle
class WDataInP final {
    const EData* m_datap;  // The backing store - non const as the handle is assignable

    // Use WDataInP::external() to create a handle from a raw pointer
    explicit WDataInP(const EData* datap) VL_PURE : m_datap{datap} {}

public:
    // FACTORY METHODS
    // Create a handle from a raw pointer - only to be used in verilated*.h
    static WDataInP external(const EData* datap) VL_PURE { return WDataInP{datap}; }

    // CONSTRUCTORS
    // Implicit conversion from 'VlWide'
    template <std::size_t N_Words>
    // cppcheck-suppress noExplicitConstructor
    /* implicit */ WDataInP(VlWide<N_Words>& vlWide) VL_PURE : m_datap{vlWide.data()} {}
    // Implicit conversion from 'const VlWide'
    template <std::size_t N_Words>
    // cppcheck-suppress noExplicitConstructor
    /* implicit */ WDataInP(const VlWide<N_Words>& vlWide) VL_PURE : m_datap{vlWide.data()} {}
    // Implicit conversion from 'WDataOutP'
    // cppcheck-suppress noExplicitConstructor
    /* implicit */ WDataInP(const WDataOutP& owp) VL_PURE : m_datap{owp.datap()} {}
    // Initialize with 'nullptr'
    explicit WDataInP(std::nullptr_t) VL_PURE : m_datap{nullptr} {}
    WDataInP(const WDataInP& other) VL_PURE = default;
    WDataInP(WDataInP&& other) VL_PURE = default;
    WDataInP& operator=(const WDataInP& other) VL_PURE = default;

    // METHODS
    const EData* datap() const VL_PURE { return m_datap; }
    operator bool() const VL_PURE { return m_datap; }
    const EData& operator[](size_t index) const VL_PURE { return m_datap[index]; }
    WDataInP operator+(size_t index) const VL_PURE { return WDataInP(m_datap + index); }
    WDataInP operator+(int index) const VL_PURE { return WDataInP(m_datap + index); }
};
static_assert(sizeof(WDataInP) == sizeof(EData*), "WDataInP should be a single pointer");

inline int _vl_cmp_w(int words, WDataInP const lwp, WDataInP const rwp) VL_PURE;

template <std::size_t N_Words>
bool VlWide<N_Words>::operator<(const VlWide<N_Words>& rhs) const VL_PURE {
    return _vl_cmp_w(N_Words, *this, rhs) < 0;
}

//===================================================================
// String formatters (required by below containers)

extern std::string VL_TO_STRING_W(int words, const WDataInP obj);

extern std::string VL_TO_STRING(CData lhs);
extern std::string VL_TO_STRING(SData lhs);
extern std::string VL_TO_STRING(IData lhs);
extern std::string VL_TO_STRING(QData lhs);
extern std::string VL_TO_STRING(double lhs);
extern std::string VL_TO_STRING(const std::string& obj) VL_PURE;
template <std::size_t N_Words>
inline std::string VL_TO_STRING(const VlWide<N_Words>& obj) {
    return VL_TO_STRING_W(N_Words, obj);
}

//=========================================================================
// Declare net data types

#ifndef VL_NO_LEGACY
#define VL_SIG8(name, msb, lsb) CData name  ///< Declare signal, 1-8 bits
#define VL_SIG16(name, msb, lsb) SData name  ///< Declare signal, 9-16 bits
#define VL_SIG64(name, msb, lsb) QData name  ///< Declare signal, 33-64 bits
#define VL_SIG(name, msb, lsb) IData name  ///< Declare signal, 17-32 bits
#define VL_SIGW(name, msb, lsb, words) VlWide<words> name  ///< Declare signal, 65+ bits
#endif

#define VL_IN8(name, msb, lsb) CData name  ///< Declare input signal, 1-8 bits
#define VL_IN16(name, msb, lsb) SData name  ///< Declare input signal, 9-16 bits
#define VL_IN64(name, msb, lsb) QData name  ///< Declare input signal, 33-64 bits
#define VL_IN(name, msb, lsb) IData name  ///< Declare input signal, 17-32 bits
#define VL_INW(name, msb, lsb, words) VlWide<words> name  ///< Declare input signal, 65+ bits
#define VL_INOUT8(name, msb, lsb) CData name  ///< Declare bidir signal, 1-8 bits
#define VL_INOUT16(name, msb, lsb) SData name  ///< Declare bidir signal, 9-16 bits
#define VL_INOUT64(name, msb, lsb) QData name  ///< Declare bidir signal, 33-64 bits
#define VL_INOUT(name, msb, lsb) IData name  ///< Declare bidir signal, 17-32 bits
#define VL_INOUTW(name, msb, lsb, words) VlWide<words> name  ///< Declare bidir signal, 65+ bits
#define VL_OUT8(name, msb, lsb) CData name  ///< Declare output signal, 1-8 bits
#define VL_OUT16(name, msb, lsb) SData name  ///< Declare output signal, 9-16 bits
#define VL_OUT64(name, msb, lsb) QData name  ///< Declare output signal, 33-64 bits
#define VL_OUT(name, msb, lsb) IData name  ///< Declare output signal, 17-32 bits
#define VL_OUTW(name, msb, lsb, words) VlWide<words> name  ///< Declare output signal, 65+ bits

//===================================================================
// Functions needed here

constexpr IData VL_CLOG2_CE_Q(QData lhs) VL_PURE {
    // constexpr usage only! Recuses to meet C++11 constexpr func limitations
    return lhs <= 1 ? 0 : VL_CLOG2_CE_Q((lhs + 1) >> 1ULL) + 1;
}

//===================================================================
// Random

// Random Number Generator with internal state
class VlRNG VL_NOT_FINAL {
    std::array<uint64_t, 2> m_state;

public:
    // The default constructor simply sets state, to avoid vl_rand64()
    // having to check for construction at each call
    // Alternative: seed with zero and check on rand64() call
    VlRNG() VL_MT_SAFE;
    explicit VlRNG(uint64_t seed) VL_PURE;
    void srandom(uint64_t n) VL_MT_UNSAFE;
    std::string get_randstate() const VL_MT_UNSAFE;
    void set_randstate(const std::string& state) VL_MT_UNSAFE;
    uint64_t rand64() VL_MT_UNSAFE;
    // Threadsafe, but requires use on vl_thread_rng or vl_current_rng
    static uint64_t vl_thread_rng_rand64() VL_MT_SAFE;
    static uint64_t vl_current_rng_rand64() VL_MT_SAFE;
    static VlRNG& vl_thread_rng() VL_MT_SAFE;
};

// VlRNG that also counts how often it was reseeded, for randomize() to notice.
class VlRNGReseeds final : public VlRNG {
    uint64_t m_reseeds = 0;  // Times the state was set from outside

public:
    void srandom(uint64_t n) VL_MT_UNSAFE {
        VlRNG::srandom(n);
        ++m_reseeds;
    }
    void set_randstate(const std::string& state) VL_MT_UNSAFE {
        VlRNG::set_randstate(state);
        ++m_reseeds;
    }
    uint64_t reseeds() const VL_MT_UNSAFE { return m_reseeds; }
};

//===================================================================
// Metadata of processes
using VlProcessRef = std::shared_ptr<VlProcess>;
class VlForkSync;
class VlForkSyncState;

class VlProcess final {
    // MEMBERS
    int m_state;  // Current state of the process
    VlProcessRef m_parentp = nullptr;  // Parent process, if exists
    std::set<VlProcess*> m_children;  // Active child processes
    std::shared_ptr<VlForkSyncState> m_forkSyncOnKillp
        = nullptr;  // Optional fork..join counter to decrement on kill
    bool m_forkSyncOnKillDone = false;  // Ensure on-kill callback fires only once
    VlRNG m_rng;  // Per-process RNG (IEEE 1800-2023 18.14)
    VlClass* m_selfp = nullptr;  // std::process object of this process, not owned

    // Thread-local current process pointer for hierarchical object seeding
    static thread_local VlProcess* t_currentp;

public:
    // TYPES
    enum : int {  // Type int for compatibility with $c
        FINISHED = 0,
        RUNNING = 1,
        WAITING = 2,
        SUSPENDED = 3,
        KILLED = 4,
    };

    // CONSTRUCTORS
    // Construct independent process
    VlProcess()
        : m_state{RUNNING} {}
    // Construct child process of parent
    explicit VlProcess(VlProcessRef parentp)
        : m_state{RUNNING}
        , m_parentp{parentp} {
        m_parentp->attach(this);
    }

    ~VlProcess() {
        if (m_parentp) m_parentp->detach(this);
        if (t_currentp == this) t_currentp = m_parentp.get();
    }

    void attach(VlProcess* childp) { m_children.insert(childp); }
    void detach(VlProcess* childp) { m_children.erase(childp); }

    int state() const { return m_state; }
    void state(int s);
    // IEEE 1800-2023 9.7: killing a completed process still terminates its live
    // descendants, but leaves the process state unchanged
    void disable() {
        if (completed()) {
            disableFork();
            return;
        }
        state(KILLED);
        disableFork();
        m_forkSyncOnKillp = nullptr;
    }
    void disableFork() {
        // childp->disable() may resume coroutines and mutate m_children
        const std::set<VlProcess*> children = m_children;
        for (VlProcess* childp : children) childp->disable();
    }
    void forkSyncOnKill(std::shared_ptr<VlForkSyncState> forkSyncp);
    void forkSyncOnKillClear(VlForkSyncState* forkSyncp);
    bool completed() const { return state() == FINISHED || state() == KILLED; }
    bool completedFork() const {
        for (const VlProcess* const childp : m_children)
            if (!childp->completed()) return false;
        return true;
    }
    template <typename T_Class>
    void initFromSelf(VlClassRef<T_Class>& selfRef) const {
        // std::process with 0 references may be waiting in VlDeleter, so don't assign it to avoid
        // use-after-free. This can happen if self() gets called after the current std::process
        // loses all references to it and gets queued for deletion.
        T_Class* const selfp = dynamic_cast<T_Class*>(m_selfp);
        if (!selfp || selfp->refCount() == 0) return;
        selfRef = VlClassRef<T_Class>{selfp};
    }
    void setSelf(VlClass* selfp) { m_selfp = selfp; }

    // Called from the std::process destructor.
    // The destroyed object may have waited in VlDeleter while self() returned null and a newer
    // object was created, so only clear m_selfp if it points to the destroyed object.
    void clearSelf(const VlClass* selfp) {
        if (m_selfp == selfp) m_selfp = nullptr;
    }

    // Random state (IEEE 1800-2023 9.7, 18.14)
    void srandom(uint64_t seed) VL_MT_UNSAFE { m_rng.srandom(seed); }
    std::string randstate() const VL_MT_UNSAFE;
    void randstate(const std::string& state) VL_MT_UNSAFE;

    // Current process tracking for hierarchical object seeding
    static VlProcess* currentp() VL_MT_UNSAFE { return t_currentp; }
    static void currentp(VlProcess* processp) VL_MT_UNSAFE { t_currentp = processp; }
    // Return process RNG if in a process, else thread RNG
    static VlRNG& currentRng() VL_MT_SAFE;
};

inline std::string VL_TO_STRING(const VlProcessRef&) { return std::string("process"); }

// Use process RNG if in a process, else thread RNG (IEEE 1800-2023 18.14)
inline uint64_t vl_rand64() VL_MT_SAFE { return VlRNG::vl_current_rng_rand64(); }

//===================================================================
// SystemVerilog event type

class VlEventBase VL_NOT_FINAL {
public:
    virtual ~VlEventBase() = default;

    virtual void fire() = 0;
    virtual bool isFired() const = 0;
    virtual bool isTriggered() const = 0;
    virtual void clearFired() = 0;
    virtual void clearTriggered() = 0;
};

class VlEvent final : public VlEventBase {
    // MEMBERS
    bool m_fired = false;  // Fired on this scheduling iteration
    bool m_triggered = false;  // Triggered state of event persisting until next time step

public:
    // CONSTRUCTOR
    VlEvent() = default;
    ~VlEvent() override = default;

    friend std::string VL_TO_STRING(const VlEvent& e);
    friend class VlAssignableEvent;
    // METHODS
    void fire() override { m_fired = m_triggered = true; }
    bool isFired() const override { return m_fired; }
    bool isTriggered() const override { return m_triggered; }
    void clearFired() override { m_fired = false; }
    void clearTriggered() override { m_triggered = false; }
};

class VlAssignableEvent final : public std::shared_ptr<VlEvent>, public VlEventBase {
public:
    // Constructor
    VlAssignableEvent()
        : std::shared_ptr<VlEvent>(new VlEvent) {}
    ~VlAssignableEvent() override = default;

    // METHODS
    void fire() override { (*this)->m_fired = (*this)->m_triggered = true; }
    bool isFired() const override { return (*this)->m_fired; }
    bool isTriggered() const override { return (*this)->m_triggered; }
    void clearFired() override { (*this)->m_fired = false; }
    void clearTriggered() override { (*this)->m_triggered = false; }
};

inline std::string VL_TO_STRING(const VlEventBase& e);

inline std::string VL_TO_STRING(const VlEvent& e) {
    return "triggered="s + (e.isTriggered() ? "true" : "false");
}

inline std::string VL_TO_STRING(const VlAssignableEvent& e) {
    return "&{ " + VL_TO_STRING(*e) + " }";
}

inline std::string VL_TO_STRING(const VlEventBase& e) {
    if (const VlAssignableEvent& assignable = dynamic_cast<const VlAssignableEvent&>(e)) {
        return VL_TO_STRING(assignable);
    }
    return "triggered="s + (e.isTriggered() ? "true" : "false");
}

// RNG for shuffle()
class VlURNG final {
public:
    using result_type = size_t;
    static constexpr size_t min() { return 0; }
    static constexpr size_t max() { return 1ULL << 31; }
    size_t operator()() { return VL_MASK_I(31) & vl_rand64(); }
};

template <typename T_Value, uint64_t N_NumValues>
class VlRandC final {
    T_Value m_remaining = 0;  // Number of values to pull before re-randomize
    T_Value m_lfsr = 1;  // LFSR state

public:
    // CONSTRUCTORS
    VlRandC() {
        static_assert(N_NumValues >= 1, "");
        static_assert(sizeof(T_Value) == 8 || (N_NumValues < (1ULL << (8 * sizeof(T_Value)))), "");
    }
    // METHODS
    T_Value randomize(VlRNG& rngr) {
        if (VL_UNLIKELY(!m_remaining)) reseed(rngr);
        // Polynomials are first listed at https://users.ece.cmu.edu/~koopman/lfsr/
        static constexpr uint64_t s_polynomials[] = {
            0x0ULL,  // 0 never used (constant, no randomization)
            0x0ULL,  // 1
            0x3ULL,        0x5ULL,       0x9ULL,        0x12ULL,       0x21ULL,
            0x41ULL,       0x8eULL,      0x108ULL,      0x204ULL,      0x402ULL,
            0x829ULL,      0x100dULL,    0x2015ULL,     0x4001ULL,
            0x8016ULL,  // 16
            0x10004ULL,    0x20040ULL,   0x40013ULL,    0x80004ULL,    0x100002ULL,
            0x200001ULL,   0x400010ULL,  0x80000dULL,   0x1000004ULL,  0x2000023ULL,
            0x4000013ULL,  0x8000004ULL, 0x10000002ULL, 0x20000029ULL, 0x40000004ULL,
            0x80000057ULL,  // 32
            0x100000029ULL  // 33
        };
        constexpr uint32_t clogWidth = VL_CLOG2_CE_Q(N_NumValues) + 1;
        constexpr uint32_t lfsrWidth = (clogWidth < 2) ? 2 : clogWidth;
        constexpr T_Value polynomial = static_cast<T_Value>(s_polynomials[lfsrWidth]);
        // printf("  numV=%ld w=%d poly=%x\n", N_NumValues, lfsrWidth, polynomial);
        //  Loop until get reasonable value. Because we picked a LFSR of at most one
        //  extra bit in width, this will only require at most on average 1.5 loops
        do {
            m_lfsr = (m_lfsr & 1ULL) ? ((m_lfsr >> 1ULL) ^ polynomial) : (m_lfsr >> 1ULL);
        } while (m_lfsr > N_NumValues);  // Note if == then output value 0
        --m_remaining;
        T_Value result = (m_lfsr == N_NumValues) ? 0 : m_lfsr;
        // printf("    result=%x  (numv=%ld, rem=%d)\n", result, N_NumValues, m_remaining);
        return result;
    }
    void reseed(VlRNG& rngr) {
        constexpr uint32_t lfsrWidth = VL_CLOG2_CE_Q(N_NumValues) + 1;
        m_remaining = N_NumValues;
        do {
            m_lfsr = rngr.rand64() & VL_MASK_Q(lfsrWidth);
            // printf("    lfsr.reseed=%x\n", m_lfsr);
        } while (!m_lfsr);  // 0 not a legal seed
    }
};

// These require the class object to have the thread safety lock
inline IData VL_RANDOM_RNG_I(VlRNG& rngr) VL_MT_UNSAFE { return rngr.rand64(); }
inline QData VL_RANDOM_RNG_Q(VlRNG& rngr) VL_MT_UNSAFE { return rngr.rand64(); }
extern double VL_RANDOM_RNG_D(VlRNG& rngr) VL_MT_UNSAFE;
extern WDataOutP VL_RANDOM_RNG_W(VlRNG& rngr, int obits, WDataOutP outwp) VL_MT_UNSAFE;

//===================================================================
// Readmem/Writemem operation classes

class VlReadMem final {
    const bool m_hex;  // Hex format
    const int m_bits;  // Bit width of values
    const std::string& m_filename;  // Filename
    const QData m_end;  // End address (as specified by user)
    const bool m_descending;  // Start address is above end, so addresses decrement
    FILE* m_fp = nullptr;  // File handle for filename
    QData m_addr = 0;  // Next address to read
    int m_linenum = 0;  // Line number last read from file
    bool m_anyAddr = false;  // Had address directive in the file
public:
    VlReadMem(bool hex, int bits, const std::string& filename, QData start, QData end);
    ~VlReadMem();
    bool isOpen() const { return m_fp != nullptr; }
    int linenum() const { return m_linenum; }
    bool get(QData& addrr, std::string& valuer);
    void setData(void* valuep, const std::string& rhs);
};

class VlWriteMem final {
    const bool m_hex;  // Hex format
    const int m_bits;  // Bit width of values
    const bool m_descending;  // Start address is above end, so addresses decrement
    FILE* m_fp = nullptr;  // File handle for filename
    QData m_addr = 0;  // Next address to write
public:
    VlWriteMem(bool hex, int bits, const std::string& filename, QData start, QData end);
    ~VlWriteMem();
    bool isOpen() const { return m_fp != nullptr; }
    void print(QData addr, bool addrstamp, const void* valuep);
};

template <typename T_Class>
class VlClassRef;

//===================================================================
// Verilog queue and dynamic array container
// There are no multithreaded locks on this; the base variable must
// be protected by other means
//
// Bound here is the maximum size() allowed, e.g. 1 + SystemVerilog bound
// For dynamic arrays it is always zero
template <typename T_Value, size_t N_MaxSize = 0>
class VlQueue;

template <typename T_Value>
class VlQueue<T_Value, 0> VL_NOT_FINAL {
public:
    using size_type = size_t;

    template <typename T_Func>
    using WithFuncReturnType = decltype(std::declval<T_Func>()(0, std::declval<T_Value>()));

protected:
    static constexpr size_type BLOCK_SIZE = sizeof(T_Value) < 512 ? 512 / sizeof(T_Value) : 1;
    struct Block final {
        typename std::aligned_storage<sizeof(T_Value), alignof(T_Value)>::type m_slots[BLOCK_SIZE];
        T_Value* at(size_type offset) { return reinterpret_cast<T_Value*>(&m_slots[offset]); }
    };
    Block** m_blocksp = nullptr;  // Array of blocks
    size_type m_mapCapacity = 0;  // Size of m_blocksp
    size_type m_firstBlock = 0;  // First used block
    size_type m_frontOffset = 0;  // Offset of the first used slot in its block
    size_type m_size = 0;  // Number of valid elements
    T_Value m_defaultValue{};  // Default value

    T_Value& element(size_type index) {
        const size_type slot = m_frontOffset + index;
        return *m_blocksp[m_firstBlock + slot / BLOCK_SIZE]->at(slot % BLOCK_SIZE);
    }
    const T_Value& element(size_type index) const {
        return const_cast<VlQueue*>(this)->element(index);
    }

    template <typename T_Compare>
    void sortCompare(T_Compare compare) {
        const auto siftDown = [this, &compare](size_type root, size_type count) {
            while (root < count / 2) {
                size_type child = root * 2 + 1;
                if (child + 1 < count && compare(element(child), element(child + 1))) ++child;
                if (!compare(element(root), element(child))) break;
                std::swap(element(root), element(child));
                root = child;
            }
        };
        for (size_type i = m_size / 2; i > 0; --i) siftDown(i - 1, m_size);
        for (size_type i = m_size; i > 1; --i) {
            std::swap(element(0), element(i - 1));
            siftDown(0, i - 1);
        }
    }

    VL_ATTR_NOINLINE void reserveForPush(bool front) {
        if (!m_size && m_mapCapacity) {
            m_firstBlock = m_mapCapacity / 2;
            m_frontOffset = BLOCK_SIZE / 2;
            return;
        }
        const size_type used = m_size ? (m_frontOffset + m_size - 1) / BLOCK_SIZE + 1 : 0;
        const size_type capacity
            = used == m_mapCapacity ? (m_mapCapacity ? m_mapCapacity * 2 : 8) : m_mapCapacity;
        const size_type first = front ? (capacity - used + 1) / 2 : (capacity - used) / 2;
        if (capacity != m_mapCapacity) {
            Block** const blocksp = new Block* [capacity] {};
            for (size_type i = 0; i < used; ++i) blocksp[first + i] = m_blocksp[m_firstBlock + i];
            delete[] m_blocksp;
            m_blocksp = blocksp;
            m_mapCapacity = capacity;
        } else {
            std::memmove(m_blocksp + first, m_blocksp + m_firstBlock, used * sizeof(Block*));
            std::fill_n(m_blocksp, first, nullptr);
            std::fill(m_blocksp + first + used, m_blocksp + capacity, nullptr);
        }
        m_firstBlock = first;
        if (!m_size) m_frontOffset = BLOCK_SIZE / 2;
    }

    VL_ATTR_NOINLINE void destroyElements() {
        for (size_type i = 0; i < m_size; ++i) element(i).~T_Value();
        const size_type used = m_size ? (m_frontOffset + m_size - 1) / BLOCK_SIZE + 1 : 0;
        for (size_type i = 0; i < used; ++i) {
            delete m_blocksp[m_firstBlock + i];
            m_blocksp[m_firstBlock + i] = nullptr;
        }
        m_size = 0;
    }

    VL_ATTR_NOINLINE void truncate(size_t size) {
        while (m_size > size) pop_back();
    }

    VL_ATTR_NOINLINE void assignFrom(const VlQueue& rhs, size_t maxSize) {
        if (this != &rhs) {
            destroyElements();
            const size_t copyCount = (maxSize && maxSize <= rhs.m_size) ? maxSize - 1 : rhs.m_size;
            for (size_t i = 0; i < copyCount; ++i) push_back(rhs.element(i));
        } else if (VL_UNLIKELY(maxSize && maxSize < m_size)) {
            truncate(maxSize - 1);
        }
    }

    VL_ATTR_NOINLINE void assignFrom(VlQueue&& rhs, size_t maxSize) {
        if (this != &rhs) {
            destroyElements();
            delete[] m_blocksp;
            m_blocksp = std::exchange(rhs.m_blocksp, nullptr);
            m_mapCapacity = std::exchange(rhs.m_mapCapacity, 0);
            m_firstBlock = std::exchange(rhs.m_firstBlock, 0);
            m_frontOffset = std::exchange(rhs.m_frontOffset, 0);
            m_size = std::exchange(rhs.m_size, 0);
        }
        if (VL_UNLIKELY(maxSize && maxSize < m_size)) truncate(maxSize - 1);
    }

public:
    // CONSTRUCTORS
    // cppcheck-suppress uninitMemberVar // m_defaultValue isn't defaulted, caller must
    VL_ATTR_NOINLINE VlQueue() {}

    VL_ATTR_NOINLINE ~VlQueue() {
        destroyElements();
        delete[] m_blocksp;
    }

    VL_ATTR_NOINLINE VlQueue(const VlQueue& rhs)
        : m_defaultValue{rhs.m_defaultValue} {
        assignFrom(rhs, 0);
    }

    VL_ATTR_NOINLINE VlQueue(VlQueue&& rhs)
        : m_defaultValue{std::move(rhs.m_defaultValue)} {
        assignFrom(std::move(rhs), 0);
    }

    VL_ATTR_NOINLINE VlQueue<T_Value, 0>& operator=(const VlQueue& rhs) {
        if (this != &rhs) {
            assignFrom(rhs, 0);
            m_defaultValue = rhs.m_defaultValue;
        }
        return *this;
    }

    VL_ATTR_NOINLINE VlQueue<T_Value, 0>& operator=(VlQueue&& rhs) {
        if (this != &rhs) {
            assignFrom(std::move(rhs), 0);
            m_defaultValue = std::move(rhs.m_defaultValue);
        }
        return *this;
    }

    // Template constuctors that construct from containers holding sub-classes
    template <typename T_Subclass>
    VL_ATTR_NOINLINE VlQueue(const VlQueue<VlClassRef<T_Subclass>, 0>&);
    template <typename T_Subclass>
    VL_ATTR_NOINLINE VlQueue(VlQueue<VlClassRef<T_Subclass>, 0>&&);

    bool operator==(const VlQueue& rhs) const {
        if (m_size != rhs.m_size) return false;
        for (size_type i = 0; i < m_size; ++i) {
            if (!(element(i) == rhs.element(i))) return false;
        }
        return true;
    }
    bool operator!=(const VlQueue& rhs) const { return !(*this == rhs); }
    bool operator<(const VlQueue& rhs) const {
        for (size_t index = 0; index < m_size; ++index) {
            if (element(index) < rhs.element(index)) return true;
        }
        return false;
    }

    // Construct new object from _V_alue and/or _C_ontainer child objects
    VL_ATTR_NOINLINE static VlQueue<T_Value, 0> consV(const T_Value& lhs, size_t maxSize = 0) {
        VlQueue out;
        out.push_back(lhs, maxSize);
        return out;
    }

    VL_ATTR_NOINLINE static VlQueue<T_Value, 0> consVV(const T_Value& lhs, const T_Value& rhs, size_t maxSize = 0) {
        VlQueue out;
        out.push_back(rhs, maxSize);
        out.push_back(lhs, maxSize);
        return out;
    }

    VL_ATTR_NOINLINE static VlQueue<T_Value, 0> consCV(const VlQueue& lhs, const T_Value& rhs, size_t maxSize = 0) {
        VlQueue out = lhs;
        out.push_front(rhs, maxSize);
        return out;
    }

    VL_ATTR_NOINLINE static VlQueue<T_Value, 0> consVC(const T_Value& lhs, const VlQueue& rhs, size_t maxSize = 0) {
        VlQueue out = rhs;
        out.push_back(lhs, maxSize);
        return out;
    }

    VL_ATTR_NOINLINE static VlQueue<T_Value, 0> consCC(const VlQueue& lhs, const VlQueue& rhs, size_t maxSize = 0) {
        VlQueue out = rhs;
        for (size_type i = 0; i < lhs.m_size; ++i) out.push_back(lhs.element(i), maxSize);
        return out;
    }

    // METHODS
    T_Value& atDefault() { return m_defaultValue; }
    const T_Value& atDefault() const { return m_defaultValue; }

    // Size. Verilog: function int size(), or int num()
    int size() const { return static_cast<int>(m_size); }
    bool empty() const { return m_size == 0; }
    // Clear array. Verilog: function void delete([input index])
    VL_ATTR_NOINLINE void clear() {
        destroyElements();
    }

    VL_ATTR_NOINLINE void erase(int32_t index) {
        if (VL_UNLIKELY(index < 0 || static_cast<size_t>(index) >= m_size)) return;
        const size_t idx = static_cast<size_t>(index);
        if (idx == 0) {
            pop_front();
            return;
        }
        for (size_t i = idx; i + 1 < m_size; ++i) element(i) = std::move(element(i + 1));
        pop_back();
    }

    // Dynamic array new[] becomes a renew()
    VL_ATTR_NOINLINE void renew(size_t size) {
        destroyElements();
        resize(size);
    }

    // Dynamic array new[]() becomes a renew_copy()
    VL_ATTR_NOINLINE void renew_copy(size_t size, const VlQueue& rhs) {
        if (size == 0) {
            clear();
        } else {
            *this = rhs;
            resize(size);
        }
    }
    // Unpacked array new[]() becomes a renew_copy()
    template <typename T_UnpackedValue, std::size_t N_UnpackedDepth>
    VL_ATTR_NOINLINE void
    renew_copy(size_t size, const VlUnpacked<T_UnpackedValue, N_UnpackedDepth>& rhs);

    VL_ATTR_NOINLINE void resize(size_t size) {
        truncate(size);
        while (m_size < size) push_back(atDefault());
    }

    // function void q.push_front(value)
    VL_ATTR_NOINLINE void push_front(const T_Value& value, size_t maxSize = 0) {
        if (VL_UNLIKELY(!m_size || (m_firstBlock == 0 && m_frontOffset == 0)))
            reserveForPush(true);
        if (m_frontOffset == 0) {
            --m_firstBlock;
            m_frontOffset = BLOCK_SIZE;
        }
        if (!m_blocksp[m_firstBlock]) m_blocksp[m_firstBlock] = new Block;
        --m_frontOffset;
        new (m_blocksp[m_firstBlock]->at(m_frontOffset)) T_Value(value);
        ++m_size;
        if (VL_UNLIKELY(maxSize != 0 && m_size > maxSize)) pop_back();
    }
    // function void q.push_back(value)
    VL_ATTR_NOINLINE void push_back(const T_Value& value, size_t maxSize = 0) {
        if (VL_LIKELY(maxSize == 0 || m_size < maxSize)) {
            if (VL_UNLIKELY(!m_size
                            || m_firstBlock + (m_frontOffset + m_size) / BLOCK_SIZE
                                   == m_mapCapacity))
                reserveForPush(false);
            const size_type slot = m_frontOffset + m_size;
            Block*& blockp = m_blocksp[m_firstBlock + slot / BLOCK_SIZE];
            if (!blockp) blockp = new Block;
            new (blockp->at(slot % BLOCK_SIZE)) T_Value(value);
            ++m_size;
        }
    }

    // function value_t q.pop_front();
    VL_ATTR_NOINLINE T_Value pop_front() {
        if (empty()) return m_defaultValue;
        T_Value v = std::move(element(0));
        element(0).~T_Value();
        --m_size;
        if (++m_frontOffset == BLOCK_SIZE || !m_size) {
            delete m_blocksp[m_firstBlock];
            m_blocksp[m_firstBlock] = nullptr;
            ++m_firstBlock;
            m_frontOffset = 0;
        }
        return v;
    }
    // function value_t q.pop_back();
    VL_ATTR_NOINLINE T_Value pop_back() {
        if (empty()) return m_defaultValue;
        T_Value v = std::move(element(m_size - 1));
        element(m_size - 1).~T_Value();
        --m_size;
        const size_type slot = m_frontOffset + m_size;
        if (slot % BLOCK_SIZE == 0 || !m_size) {
            const size_type block = m_firstBlock + slot / BLOCK_SIZE;
            delete m_blocksp[block];
            m_blocksp[block] = nullptr;
        }
        return v;
    }

    // Setting. Verilog: assoc[index] = v (should only be used by dynamic arrays)
    VL_ATTR_NOINLINE T_Value& atWrite(int32_t index) {
        // cppcheck-suppress variableScope
        static thread_local T_Value t_throwAway;
        // Needs to work for dynamic arrays, so does not use N_MaxSize
        if (VL_UNLIKELY(index < 0 || static_cast<size_t>(index) >= m_size)) {
            t_throwAway = atDefault();
            return t_throwAway;
        }
        return element(index);
    }
    // Setting. Verilog: assoc[index] = v (should only be used by queues)
    VL_ATTR_NOINLINE T_Value& atWriteAppend(int32_t index, size_t maxSize = 0) {
        // cppcheck-suppress variableScope
        static thread_local T_Value t_throwAway;
        if (static_cast<size_t>(index) == m_size) push_back(atDefault(), maxSize);
        if (VL_UNLIKELY(index < 0 || static_cast<size_t>(index) >= m_size)) {
            t_throwAway = atDefault();
            return t_throwAway;
        }
        return element(index);
    }
    // Accessing. Verilog: v = assoc[index]
    const T_Value& at(int32_t index) const {
        // Needs to work for dynamic arrays, so does not use N_MaxSize
        if (VL_UNLIKELY(index < 0 || static_cast<size_t>(index) >= m_size)) return atDefault();
        return element(index);
    }

    // Access with an index counted from end (e.g. q[$])
    T_Value& atWriteAppendBack(int32_t index, size_t maxSize = 0) {
        return atWriteAppend(m_size - 1 - index, maxSize);
    }
    const T_Value& atBack(int32_t index) const { return at(m_size - 1 - index); }

    // function void q.insert(index, value);
    VL_ATTR_NOINLINE void insert(int32_t index, const T_Value& value, size_t maxSize = 0) {
        if (VL_UNLIKELY(index < 0 || static_cast<size_t>(index) > m_size)) return;
        if (VL_UNLIKELY(maxSize != 0 && m_size >= maxSize)) return;
        const size_t idx = static_cast<size_t>(index);
        if (idx == 0) {
            push_front(value);
            return;
        }
        if (idx == m_size) {
            push_back(value);
            return;
        }
        const T_Value copy = value;
        push_back(element(m_size - 1));
        for (size_t i = m_size - 2; i > idx; --i) element(i) = std::move(element(i - 1));
        element(idx) = copy;
    }

    // inside (set membership operator)
    bool inside(const T_Value& value) const {
        for (size_type i = 0; i < m_size; ++i) {
            if (element(i) == value) return true;
        }
        return false;
    }

    // Return slice q[lsb:msb]
    VL_ATTR_NOINLINE VlQueue<T_Value, 0> slice(int32_t lsb, int32_t msb, size_t maxSize = 0) const {
        VlQueue out;
        if (VL_UNLIKELY(lsb < 0)) lsb = 0;
        if (VL_UNLIKELY(lsb >= static_cast<int32_t>(m_size)))
            lsb = static_cast<int32_t>(m_size) - 1;
        if (VL_UNLIKELY(msb >= static_cast<int32_t>(m_size)))
            msb = static_cast<int32_t>(m_size) - 1;
        for (int32_t i = lsb; i <= msb; ++i) out.push_back(element(i), maxSize);
        return out;
    }
    VlQueue sliceFrontBack(int32_t lsb, int32_t msb, size_t maxSize = 0) const {
        return slice(lsb, m_size - 1 - msb, maxSize);
    }
    VlQueue sliceBackBack(int32_t lsb, int32_t msb, size_t maxSize = 0) const {
        return slice(m_size - 1 - lsb, m_size - 1 - msb, maxSize);
    }
    // Assign src elements to q[lsb:msb]
    VL_ATTR_NOINLINE void sliceAssign(int32_t lsb, int32_t msb, const VlQueue& src) {
        const int32_t sz = static_cast<int32_t>(m_size);
        const int32_t srcSz = static_cast<int32_t>(src.m_size);
        if (VL_UNLIKELY(sz <= 0 || srcSz <= 0)) return;
        if (VL_UNLIKELY(lsb < 0)) lsb = 0;
        if (VL_UNLIKELY(lsb >= sz)) lsb = sz - 1;
        if (VL_UNLIKELY(msb >= sz)) msb = sz - 1;
        const int32_t count = std::min(msb - lsb + 1, srcSz);
        if (VL_UNLIKELY(count <= 0)) return;
        for (int32_t i = 0; i < count; ++i) element(lsb + i) = src.element(i);
    }
    void sliceAssignFrontBack(int32_t lsb, int32_t msb, const VlQueue& src) {
        sliceAssign(lsb, m_size - 1 - msb, src);
    }
    void sliceAssignBackBack(int32_t lsb, int32_t msb, const VlQueue& src) {
        sliceAssign(m_size - 1 - lsb, m_size - 1 - msb, src);
    }

    // Methods
    VL_ATTR_NOINLINE void sort() {
        sortCompare([](const T_Value& a, const T_Value& b) { return a < b; });
    }

    template <typename T_Func>
    void sort(T_Func with_func) {
        // with_func returns arbitrary type to use for the sort comparison
        sortCompare([=](const T_Value& a, const T_Value& b) {
            // index number is meaningless with sort, as it changes
            return with_func(0, a) < with_func(0, b);
        });
    }

    VL_ATTR_NOINLINE void rsort() {
        sortCompare([](const T_Value& a, const T_Value& b) { return b < a; });
    }

    template <typename T_Func>
    void rsort(T_Func with_func) {
        // with_func returns arbitrary type to use for the sort comparison
        sortCompare([=](const T_Value& a, const T_Value& b) {
            // index number is meaningless with sort, as it changes
            return with_func(0, b) < with_func(0, a);
        });
    }

    VL_ATTR_NOINLINE void reverse() {
        for (size_type i = 0; i < m_size / 2; ++i) {
            std::swap(element(i), element(m_size - 1 - i));
        }
    }
    VL_ATTR_NOINLINE void shuffle() {
        VlURNG rng;
        for (size_type i = m_size; i > 1; --i) {
            std::uniform_int_distribution<size_type> dist{0, i - 1};
            std::swap(element(i - 1), element(dist(rng)));
        }
    }
    VL_ATTR_NOINLINE VlQueue<T_Value, 0> unique() const {
        VlQueue out;
        std::set<T_Value> saw;
        for (size_type index = 0; index < m_size; ++index) {
            const auto& i = element(index);
            if (saw.insert(i).second) { out.push_back(i); }
        }
        return out;
    }

    template <typename T_Func>
    VlQueue unique(T_Func with_func) const {
        VlQueue out;
        std::set<decltype(with_func(0, element(0)))> saw;
        for (size_type index = 0; index < m_size; ++index) {
            const auto& i = element(index);
            const auto i_mapped = with_func(0, i);
            if (saw.insert(i_mapped).second) { out.push_back(i); }
        }
        return out;
    }

    VL_ATTR_NOINLINE VlQueue<IData, 0> unique_index() const {
        VlQueue<IData, 0> out;
        std::set<T_Value> saw;
        for (size_type index = 0; index < m_size; ++index) {
            const auto& i = element(index);
            if (saw.insert(i).second) { out.push_back(index); }
        }
        return out;
    }

    template <typename T_Func>
    VlQueue<IData, 0> unique_index(T_Func with_func) const {
        VlQueue<IData, 0> out;
        std::set<decltype(with_func(0, element(0)))> saw;
        for (size_type index = 0; index < m_size; ++index) {
            const auto& i = element(index);
            const auto i_mapped = with_func(static_cast<IData>(index), i);
            if (saw.insert(i_mapped).second) { out.push_back(index); }
        }
        return out;
    }
    template <typename T_Func>
    VlQueue find(T_Func with_func) const {
        VlQueue out;
        for (size_type index = 0; index < m_size; ++index) {
            const auto& i = element(index);
            if (with_func(static_cast<IData>(index), i)) out.push_back(i);
        }
        return out;
    }
    template <typename T_Func>
    VlQueue<IData, 0> find_index(T_Func with_func) const {
        VlQueue<IData, 0> out;
        for (size_type index = 0; index < m_size; ++index) {
            const auto& i = element(index);
            if (with_func(static_cast<IData>(index), i)) out.push_back(index);
        }
        return out;
    }
    template <typename T_Func>
    VlQueue find_first(T_Func with_func) const {
        for (size_type index = 0; index < m_size; ++index) {
            const auto& i = element(index);
            if (with_func(static_cast<IData>(index), i)) return VlQueue::consV(i);
        }
        return VlQueue{};
    }
    template <typename T_Func>
    VlQueue<IData, 0> find_first_index(T_Func with_func) const {
        for (size_type index = 0; index < m_size; ++index) {
            const auto& i = element(index);
            if (with_func(static_cast<IData>(index), i)) return VlQueue<IData, 0>::consV(index);
        }
        return VlQueue<IData, 0>{};
    }
    template <typename T_Func>
    VlQueue find_last(T_Func with_func) const {
        for (int64_t index = static_cast<int64_t>(m_size) - 1; index >= 0; --index) {
            const auto& item = element(index);
            if (with_func(index, item)) return VlQueue::consV(item);
        }
        return VlQueue{};
    }
    template <typename T_Func>
    VlQueue<IData, 0> find_last_index(T_Func with_func) const {
        for (int64_t index = static_cast<int64_t>(m_size) - 1; index >= 0; --index) {
            const auto& item = element(index);
            if (with_func(index, item)) return VlQueue<IData, 0>::consV(index);
        }
        return VlQueue<IData, 0>{};
    }
    // Map method (IEEE 1800-2023 7.12.5)
    template <typename T_Func>
    VlQueue<WithFuncReturnType<T_Func>, 0> map(T_Func with_func) const {
        VlQueue<WithFuncReturnType<T_Func>, 0> out;
        for (size_type index = 0; index < m_size; ++index) {
            const auto& i = element(index);
            out.push_back(with_func(static_cast<IData>(index), i));
        }
        return out;
    }

    // Reduction operators
    VL_ATTR_NOINLINE VlQueue<T_Value, 0> min() const {
        if (empty()) return VlQueue{};
        size_type best = 0;
        for (size_type index = 1; index < m_size; ++index) {
            if (element(index) < element(best)) best = index;
        }
        return VlQueue::consV(element(best));
    }

    template <typename T_Func>
    VlQueue min(T_Func with_func) const {
        if (empty()) return VlQueue{};
        size_type best = 0;
        for (size_type index = 1; index < m_size; ++index) {
            if (with_func(0, element(index)) < with_func(0, element(best))) best = index;
        }
        return VlQueue::consV(element(best));
    }

    VL_ATTR_NOINLINE VlQueue<T_Value, 0> max() const {
        if (empty()) return VlQueue{};
        size_type best = 0;
        for (size_type index = 1; index < m_size; ++index) {
            if (element(best) < element(index)) best = index;
        }
        return VlQueue::consV(element(best));
    }

    template <typename T_Func>
    VlQueue max(T_Func with_func) const {
        if (empty()) return VlQueue{};
        size_type best = 0;
        for (size_type index = 1; index < m_size; ++index) {
            if (with_func(0, element(best)) < with_func(0, element(index))) best = index;
        }
        return VlQueue::consV(element(best));
    }

    template <typename U = T_Value>
    T_Value r_sum() const {
        T_Value out(0);  // Type must have assignment operator
        for (size_type index = 0; index < m_size; ++index) out += element(index);
        return out;
    }
    template <typename T_Func>
    WithFuncReturnType<T_Func> r_sum(T_Func with_func) const {
        WithFuncReturnType<T_Func> out = WithFuncReturnType<T_Func>(0);
        for (size_type index = 0; index < m_size; ++index) {
            out += with_func(static_cast<IData>(index), element(index));
        }
        return out;
    }
    template <typename U = T_Value>
    T_Value r_product() const {
        if (empty()) return T_Value(0);  // The big three do it this way
        T_Value out = T_Value(1);
        for (size_type index = 0; index < m_size; ++index) out *= element(index);
        return out;
    }
    template <typename T_Func>
    WithFuncReturnType<T_Func> r_product(T_Func with_func) const {
        if (empty()) return WithFuncReturnType<T_Func>(0);  // The big three do it this way
        WithFuncReturnType<T_Func> out = WithFuncReturnType<T_Func>(1);
        for (size_type index = 0; index < m_size; ++index) {
            out *= with_func(static_cast<IData>(index), element(index));
        }
        return out;
    }
    template <typename U = T_Value>
    T_Value r_and() const {
        if (empty()) return T_Value(0);  // The big three do it this way
        T_Value out = ~T_Value(0);
        for (size_type index = 0; index < m_size; ++index) out &= element(index);
        return out;
    }
    template <typename T_Func>
    WithFuncReturnType<T_Func> r_and(T_Func with_func) const {
        if (empty()) return WithFuncReturnType<T_Func>(0);  // The big three do it this way
        WithFuncReturnType<T_Func> out = ~WithFuncReturnType<T_Func>(0);
        for (size_type index = 0; index < m_size; ++index) {
            out &= with_func(static_cast<IData>(index), element(index));
        }
        return out;
    }
    template <typename U = T_Value>
    T_Value r_or() const {
        T_Value out = T_Value(0);
        for (size_type index = 0; index < m_size; ++index) out |= element(index);
        return out;
    }
    template <typename T_Func>
    WithFuncReturnType<T_Func> r_or(T_Func with_func) const {
        WithFuncReturnType<T_Func> out = WithFuncReturnType<T_Func>(0);
        for (size_type index = 0; index < m_size; ++index) {
            out |= with_func(static_cast<IData>(index), element(index));
        }
        return out;
    }
    template <typename U = T_Value>
    T_Value r_xor() const {
#ifdef VERILATOR_BIG3_NULLARY_ARITHMETICS_QUIRKS
        if (empty()) return T_Value(0);
#endif
        T_Value out = T_Value(0);
        for (size_type index = 0; index < m_size; ++index) out ^= element(index);
        return out;
    }
    template <typename T_Func>
    WithFuncReturnType<T_Func> r_xor(T_Func with_func) const {
        WithFuncReturnType<T_Func> out = WithFuncReturnType<T_Func>(0);
        for (size_type index = 0; index < m_size; ++index) {
            out ^= with_func(static_cast<IData>(index), element(index));
        }
        return out;
    }

    // Dumping. Verilog: str = $sformatf("%p", assoc)
    VL_ATTR_NOINLINE std::string to_string() const {
        if (empty()) return "'{}";  // No trailing space
        std::string out = "'{";
        std::string comma;
        for (size_type index = 0; index < m_size; ++index) {
            const auto& i = element(index);
            out += comma + VL_TO_STRING(i);
            comma = ", ";
        }
        return out + "}";
    }
};

template <typename T_Value, size_t N_MaxSize>
class VlQueue final : public VlQueue<T_Value, 0> {
private:
    using Base = VlQueue<T_Value, 0>;

public:
    // CONSTRUCTORS
    VL_ATTR_ALWINLINE VlQueue() = default;
    VL_ATTR_ALWINLINE ~VlQueue() = default;
    VL_ATTR_ALWINLINE VlQueue(const VlQueue&) = default;
    VL_ATTR_ALWINLINE VlQueue(VlQueue&&) = default;
    // cppcheck-suppress noExplicitConstructor
    VL_ATTR_ALWINLINE VlQueue(const Base& rhs)
        : Base{rhs} {
        if (VL_UNLIKELY(N_MaxSize < this->m_size)) this->truncate(N_MaxSize - 1);
    }
    // cppcheck-suppress noExplicitConstructor
    VL_ATTR_ALWINLINE VlQueue(Base&& rhs)
        : Base{std::move(rhs)} {
        if (VL_UNLIKELY(N_MaxSize < this->m_size)) this->truncate(N_MaxSize - 1);
    }
    VL_ATTR_ALWINLINE VlQueue& operator=(const VlQueue&) = default;
    VL_ATTR_ALWINLINE VlQueue& operator=(VlQueue&&) = default;
    VL_ATTR_ALWINLINE VlQueue& operator=(const Base& rhs) {
        this->assignFrom(rhs, N_MaxSize);
        return *this;
    }
    VL_ATTR_ALWINLINE VlQueue& operator=(Base&& rhs) {
        this->assignFrom(std::move(rhs), N_MaxSize);
        return *this;
    }

    // Template constuctors that construct from containers holding sub-classes
    template <typename T_Subclass>
    VL_ATTR_ALWINLINE VlQueue(const VlQueue<VlClassRef<T_Subclass>, 0>& that)
        : Base{that} {
        if (VL_UNLIKELY(N_MaxSize < this->m_size)) this->truncate(N_MaxSize);
    }
    template <typename T_Subclass>
    VL_ATTR_ALWINLINE VlQueue(VlQueue<VlClassRef<T_Subclass>, 0>&& that)
        : Base{std::move(that)} {
        if (VL_UNLIKELY(N_MaxSize < this->m_size)) this->truncate(N_MaxSize);
    }

    // Construct new object from _V_alue and/or _C_ontainer child objects
    VL_ATTR_ALWINLINE static Base consV(const T_Value& lhs) {
        return Base::consV(lhs, N_MaxSize);
    }
    VL_ATTR_ALWINLINE static Base consVV(const T_Value& lhs, const T_Value& rhs) {
        return Base::consVV(lhs, rhs, N_MaxSize);
    }
    VL_ATTR_ALWINLINE static Base consCV(const Base& lhs, const T_Value& rhs) {
        return Base::consCV(lhs, rhs, N_MaxSize);
    }
    VL_ATTR_ALWINLINE static Base consVC(const T_Value& lhs, const Base& rhs) {
        return Base::consVC(lhs, rhs, N_MaxSize);
    }
    VL_ATTR_ALWINLINE static Base consCC(const Base& lhs, const Base& rhs) {
        return Base::consCC(lhs, rhs, N_MaxSize);
    }

    // METHODS
    VL_ATTR_ALWINLINE void push_front(const T_Value& value) {
        Base::push_front(value, N_MaxSize);
    }
    VL_ATTR_ALWINLINE void push_back(const T_Value& value) {
        Base::push_back(value, N_MaxSize);
    }
    VL_ATTR_ALWINLINE void insert(int32_t index, const T_Value& value) {
        Base::insert(index, value, N_MaxSize);
    }
    VL_ATTR_ALWINLINE T_Value& atWriteAppend(int32_t index) {
        return Base::atWriteAppend(index, N_MaxSize);
    }
    VL_ATTR_ALWINLINE T_Value& atWriteAppendBack(int32_t index) {
        return Base::atWriteAppendBack(index, N_MaxSize);
    }
    VL_ATTR_ALWINLINE Base slice(int32_t lsb, int32_t msb) const {
        return Base::slice(lsb, msb, N_MaxSize);
    }
    VL_ATTR_ALWINLINE Base sliceFrontBack(int32_t lsb, int32_t msb) const {
        return Base::sliceFrontBack(lsb, msb, N_MaxSize);
    }
    VL_ATTR_ALWINLINE Base sliceBackBack(int32_t lsb, int32_t msb) const {
        return Base::sliceBackBack(lsb, msb, N_MaxSize);
    }
};

template <typename T_Value, size_t N_MaxSize>
std::string VL_TO_STRING(const VlQueue<T_Value, N_MaxSize>& obj) {
    return obj.to_string();
}

template <typename T_Value, size_t N_MaxSize>
struct VlContainsCustomStruct<VlQueue<T_Value, N_MaxSize>> : VlContainsCustomStruct<T_Value> {};

//===================================================================
// Verilog associative array container
// There are no multithreaded locks on this; the base variable must
// be protected by other means
//
template <typename T_Key, typename T_Value>
class VlAssocArray final {
private:
    // TYPES
    using Map = std::map<T_Key, T_Value>;

public:
    using const_iterator = typename Map::const_iterator;
    template <typename T_Func>
    using WithFuncReturnType
        = decltype(std::declval<T_Func>()(std::declval<T_Key>(), std::declval<T_Value>()));

private:
    // MEMBERS
    Map m_map;  // State of the assoc array
    T_Value m_defaultValue;  // Default value

public:
    // CONSTRUCTORS
    // m_defaultValue isn't defaulted. Caller's constructor must do it.
    VlAssocArray() = default;
    // Construct with the given entries
    explicit VlAssocArray(std::initializer_list<std::pair<const T_Key, T_Value>> init)
        : m_map{init}
        , m_defaultValue{} {}
    // Construct with the given default value and entries
    explicit VlAssocArray(const T_Value& defaultValue,
                          std::initializer_list<std::pair<const T_Key, T_Value>> init)
        : m_map{init}
        , m_defaultValue{defaultValue} {}
    ~VlAssocArray() = default;
    VlAssocArray(const VlAssocArray&) = default;
    VlAssocArray(VlAssocArray&&) = default;
    VlAssocArray& operator=(const VlAssocArray&) = default;
    VlAssocArray& operator=(VlAssocArray&&) = default;
    bool operator==(const VlAssocArray& rhs) const { return m_map == rhs.m_map; }
    bool operator!=(const VlAssocArray& rhs) const { return m_map != rhs.m_map; }
    bool operator<(const VlAssocArray& rhs) const { return m_map < rhs.m_map; }
    // METHODS
    T_Value& atDefault() { return m_defaultValue; }
    const T_Value& atDefault() const { return m_defaultValue; }

    // Size of array. Verilog: function int size(), or int num()
    int size() const { return m_map.size(); }
    bool empty() const { return m_map.empty(); }
    // Clear array. Verilog: function void delete([input index])
    void clear() { m_map.clear(); }
    void erase(const T_Key& index) { m_map.erase(index); }
    // Return 0/1 if element exists. Verilog: function int exists(input index)
    int exists(const T_Key& index) const { return m_map.find(index) != m_map.end(); }
    // Return first element.  Verilog: function int first(ref index);
    int first(T_Key& indexr) const {
        const auto it = m_map.cbegin();
        if (it == m_map.end()) return 0;
        indexr = it->first;
        return 1;
    }
    // Return last element.  Verilog: function int last(ref index)
    int last(T_Key& indexr) const {
        const auto it = m_map.crbegin();
        if (it == m_map.crend()) return 0;
        indexr = it->first;
        return 1;
    }
    // Return next element. Verilog: function int next(ref index)
    int next(T_Key& indexr) const {
        auto it = m_map.find(indexr);
        if (VL_UNLIKELY(it == m_map.end())) return 0;
        ++it;
        if (VL_UNLIKELY(it == m_map.end())) return 0;
        indexr = it->first;
        return 1;
    }
    // Return prev element. Verilog: function int prev(ref index)
    int prev(T_Key& indexr) const {
        auto it = m_map.find(indexr);
        if (VL_UNLIKELY(it == m_map.end())) return 0;
        if (VL_UNLIKELY(it == m_map.begin())) return 0;
        --it;
        indexr = it->first;
        return 1;
    }
    // Setting. Verilog: assoc[index] = v
    T_Value& atWrite(const T_Key& index) {
        const auto it = m_map.find(index);
        if (it == m_map.end()) {
            std::pair<typename Map::iterator, bool> pit = m_map.emplace(index, m_defaultValue);
            return pit.first->second;
        }
        return it->second;
    }
    // Accessing. Verilog: v = assoc[index]
    const T_Value& at(const T_Key& index) const {
        const auto it = m_map.find(index);
        if (it == m_map.end()) return m_defaultValue;
        return it->second;
    }
    // Setting as a chained operation
    VlAssocArray& set(const T_Key& index, const T_Value& value) {
        atWrite(index) = value;
        return *this;
    }
    VlAssocArray& setDefault(const T_Value& value) {
        atDefault() = value;
        return *this;
    }

    // For save/restore
    const_iterator begin() const { return m_map.begin(); }
    const_iterator end() const { return m_map.end(); }

    // Methods
    VlQueue<T_Value> unique() const {
        VlQueue<T_Value> out;
        std::set<T_Value> saw;
        for (const auto& i : m_map) {
            auto it = saw.find(i.second);
            if (it == saw.end()) {
                saw.insert(it, i.second);
                out.push_back(i.second);
            }
        }
        return out;
    }
    template <typename T_Func>
    VlQueue<T_Value> unique(T_Func with_func) const {
        VlQueue<T_Value> out;
        T_Key default_key;
        using WithType = decltype(with_func(m_map.begin()->first, m_map.begin()->second));
        std::set<WithType> saw;
        for (const auto& i : m_map) {
            const auto i_mapped = with_func(default_key, i.second);
            const auto it = saw.find(i_mapped);
            if (it == saw.end()) {
                saw.insert(it, i_mapped);
                out.push_back(i.second);
            }
        }
        return out;
    }
    VlQueue<T_Key> unique_index() const {
        VlQueue<T_Key> out;
        std::set<T_Value> saw;
        for (const auto& i : m_map) {
            auto it = saw.find(i.second);
            if (it == saw.end()) {
                saw.insert(it, i.second);
                out.push_back(i.first);
            }
        }
        return out;
    }
    template <typename T_Func>
    VlQueue<T_Key> unique_index(T_Func with_func) const {
        VlQueue<T_Key> out;
        using WithType = decltype(with_func(m_map.begin()->first, m_map.begin()->second));
        std::set<WithType> saw;
        for (const auto& i : m_map) {
            const auto i_mapped = with_func(i.first, i.second);
            auto it = saw.find(i_mapped);
            if (it == saw.end()) {
                saw.insert(it, i_mapped);
                out.push_back(i.first);
            }
        }
        return out;
    }
    template <typename T_Func>
    VlQueue<T_Value> find(T_Func with_func) const {
        VlQueue<T_Value> out;
        for (const auto& i : m_map)
            if (with_func(i.first, i.second)) out.push_back(i.second);
        return out;
    }
    template <typename T_Func>
    VlQueue<T_Key> find_index(T_Func with_func) const {
        VlQueue<T_Key> out;
        for (const auto& i : m_map)
            if (with_func(i.first, i.second)) out.push_back(i.first);
        return out;
    }
    template <typename T_Func>
    VlQueue<T_Value> find_first(T_Func with_func) const {
        const auto it
            = std::find_if(m_map.cbegin(), m_map.cend(), [=](const std::pair<T_Key, T_Value>& i) {
                  return with_func(i.first, i.second);
              });
        if (it == m_map.end()) return VlQueue<T_Value>{};
        return VlQueue<T_Value>::consV(it->second);
    }
    template <typename T_Func>
    VlQueue<T_Key> find_first_index(T_Func with_func) const {
        const auto it
            = std::find_if(m_map.cbegin(), m_map.cend(), [=](const std::pair<T_Key, T_Value>& i) {
                  return with_func(i.first, i.second);
              });
        if (it == m_map.end()) return VlQueue<T_Key>{};
        return VlQueue<T_Key>::consV(it->first);
    }
    template <typename T_Func>
    VlQueue<T_Value> find_last(T_Func with_func) const {
        const auto it = std::find_if(
            m_map.crbegin(), m_map.crend(),
            [=](const std::pair<T_Key, T_Value>& i) { return with_func(i.first, i.second); });
        if (it == m_map.rend()) return VlQueue<T_Value>{};
        return VlQueue<T_Value>::consV(it->second);
    }
    template <typename T_Func>
    VlQueue<T_Key> find_last_index(T_Func with_func) const {
        const auto it = std::find_if(
            m_map.crbegin(), m_map.crend(),
            [=](const std::pair<T_Key, T_Value>& i) { return with_func(i.first, i.second); });
        if (it == m_map.rend()) return VlQueue<T_Key>{};
        return VlQueue<T_Key>::consV(it->first);
    }
    // Map method (IEEE 1800-2023 7.12.5)
    template <typename T_Func>
    VlQueue<WithFuncReturnType<T_Func>> map(T_Func with_func) const {
        VlQueue<WithFuncReturnType<T_Func>> out;
        for (const auto& i : m_map) out.push_back(with_func(i.first, i.second));
        return out;
    }

    // Reduction operators
    VlQueue<T_Value> min() const {
        if (m_map.empty()) return VlQueue<T_Value>();
        const auto it = std::min_element(
            m_map.cbegin(), m_map.cend(),
            [](const std::pair<T_Key, T_Value>& a, const std::pair<T_Key, T_Value>& b) {
                return a.second < b.second;
            });
        return VlQueue<T_Value>::consV(it->second);
    }
    template <typename T_Func>
    VlQueue<T_Value> min(T_Func with_func) const {
        if (m_map.empty()) return VlQueue<T_Value>();
        const auto it = std::min_element(
            m_map.cbegin(), m_map.cend(),
            [&with_func](const std::pair<T_Key, T_Value>& a, const std::pair<T_Key, T_Value>& b) {
                return with_func(a.first, a.second) < with_func(b.first, b.second);
            });
        return VlQueue<T_Value>::consV(it->second);
    }
    VlQueue<T_Value> max() const {
        if (m_map.empty()) return VlQueue<T_Value>();
        const auto it = std::max_element(
            m_map.cbegin(), m_map.cend(),
            [](const std::pair<T_Key, T_Value>& a, const std::pair<T_Key, T_Value>& b) {
                return a.second < b.second;
            });
        return VlQueue<T_Value>::consV(it->second);
    }
    template <typename T_Func>
    VlQueue<T_Value> max(T_Func with_func) const {
        if (m_map.empty()) return VlQueue<T_Value>();
        const auto it = std::max_element(
            m_map.cbegin(), m_map.cend(),
            [&with_func](const std::pair<T_Key, T_Value>& a, const std::pair<T_Key, T_Value>& b) {
                return with_func(a.first, a.second) < with_func(b.first, b.second);
            });
        return VlQueue<T_Value>::consV(it->second);
    }

    T_Value r_sum() const {
        T_Value out = T_Value{};
        for (const auto& i : m_map) out += i.second;
        return out;
    }
    template <typename T_Func>
    WithFuncReturnType<T_Func> r_sum(T_Func with_func) const {
        WithFuncReturnType<T_Func> out = WithFuncReturnType<T_Func>(0);
        for (const auto& i : m_map) out += with_func(i.first, i.second);
        return out;
    }
    T_Value r_product() const {
        // The big three return 0 when assoc array is empty
        if (m_map.empty()) return T_Value{};
        T_Value out = T_Value{1};
        for (const auto& i : m_map) out *= i.second;
        return out;
    }
    template <typename T_Func>
    WithFuncReturnType<T_Func> r_product(T_Func with_func) const {
        if (m_map.empty()) return WithFuncReturnType<T_Func>(0);  // The big three do it this way
        WithFuncReturnType<T_Func> out = WithFuncReturnType<T_Func>(1);
        for (const auto& i : m_map) out *= with_func(i.first, i.second);
        return out;
    }
    T_Value r_and() const {
        // The big three return 0 when assoc array is empty
        if (m_map.empty()) return T_Value{};
        T_Value out = m_map.cbegin()->second;
        for (const auto& i : m_map) out &= i.second;
        return out;
    }
    template <typename T_Func>
    WithFuncReturnType<T_Func> r_and(T_Func with_func) const {
        if (m_map.empty()) return WithFuncReturnType<T_Func>(0);  // The big three do it this way
        WithFuncReturnType<T_Func> out = ~WithFuncReturnType<T_Func>(0);
        for (const auto& i : m_map) out &= with_func(i.first, i.second);
        return out;
    }
    T_Value r_or() const {
        T_Value out = T_Value{};
        for (const auto& i : m_map) out |= i.second;
        return out;
    }
    template <typename T_Func>
    WithFuncReturnType<T_Func> r_or(T_Func with_func) const {
        WithFuncReturnType<T_Func> out = WithFuncReturnType<T_Func>(0);
        for (const auto& i : m_map) out |= with_func(i.first, i.second);
        return out;
    }
    T_Value r_xor() const {
        T_Value out = T_Value{};
        for (const auto& i : m_map) out ^= i.second;
        return out;
    }
    template <typename T_Func>
    WithFuncReturnType<T_Func> r_xor(T_Func with_func) const {
        WithFuncReturnType<T_Func> out = WithFuncReturnType<T_Func>(0);
        for (const auto& i : m_map) out ^= with_func(i.first, i.second);
        return out;
    }

    // Dumping. Verilog: str = $sformatf("%p", assoc)
    std::string to_string() const {
        if (m_map.empty()) return "'{}";  // No trailing space
        std::string out = "'{";
        std::string comma;
        for (const auto& i : m_map) {
            out += comma + VL_TO_STRING(i.first) + ":" + VL_TO_STRING(i.second);
            comma = ", ";
        }
        // Default not printed - maybe random init data
        return out + "}";
    }
};

template <typename T_Key, typename T_Value>
std::string VL_TO_STRING(const VlAssocArray<T_Key, T_Value>& obj) {
    return obj.to_string();
}

template <typename T_Key, typename T_Value>
struct VlContainsCustomStruct<VlAssocArray<T_Key, T_Value>> : VlContainsCustomStruct<T_Value> {};

template <typename T_Key, typename T_Value>
void VL_READMEM_N(bool hex, int bits, const std::string& filename,
                  VlAssocArray<T_Key, T_Value>& obj, QData start, QData end) VL_MT_SAFE {
    VlReadMem rmem{hex, bits, filename, start, end};
    if (VL_UNLIKELY(!rmem.isOpen())) return;
    while (true) {
        QData addr;
        std::string data;
        if (rmem.get(addr /*ref*/, data /*ref*/)) {
            rmem.setData(&(obj.atWrite(addr)), data);
        } else {
            break;
        }
    }
}

template <typename T_Key, typename T_Value>
void VL_WRITEMEM_N(bool hex, int bits, const std::string& filename,
                   const VlAssocArray<T_Key, T_Value>& obj, QData start, QData end) VL_MT_SAFE {
    VlWriteMem wmem{hex, bits, filename, start, end};
    if (VL_UNLIKELY(!wmem.isOpen())) return;
    const QData lo = std::min(start, end);
    const QData hi = std::max(start, end);
    const auto printIfInRange
        = [&](const typename VlAssocArray<T_Key, T_Value>::const_iterator& it) {
              const QData addr = it->first;
              if (addr >= lo && addr <= hi) wmem.print(addr, true, &(it->second));
          };
    if (start > end) {
        for (auto it = obj.end(); it != obj.begin();) printIfInRange(--it);
    } else {
        for (auto it = obj.begin(); it != obj.end();) printIfInRange(it++);
    }
}

//===================================================================
/// Verilog unpacked array container
/// For when a standard C++[] array is not sufficient, e.g. an
/// array under a queue, or methods operating on the array.
///
/// A 'struct' as we want this to be an aggregate type that allows
/// static aggregate initialization. Consider data members private.
///
/// This class may get exposed to a Verilated Model's top I/O, if the top
/// IO has an unpacked array.

template <typename T_Value, std::size_t N_Depth>
class VlUnpacked final {
    // TYPES
    using T_Key = IData;  // Index type, for uniformity with other containers
    using Unpacked = T_Value[N_Depth];

public:
    template <typename T_Func>
    using WithFuncReturnType = decltype(std::declval<T_Func>()(0, std::declval<T_Value>()));

    // MEMBERS
    // This should be the only data member, otherwise generated static initializers need updating
    Unpacked m_storage;  // Contents of the unpacked array

    // CONSTRUCTORS
    // Default constructors and destructor are used. Note however that C++20 requires that
    // aggregate types do not have a user declared constructor, not even an explicitly defaulted
    // one.

    // OPERATOR METHODS
    // Default copy assignment operators are used.

    // METHODS
    // Raw access
    T_Value* data() { return &m_storage[0]; }
    const T_Value* data() const { return &m_storage[0]; }

    constexpr std::size_t size() const { return N_Depth; }

    // Runtime slice v[loIdx +: N_Out], loIdx being an index into m_storage
    template <std::size_t N_Out>
    VlUnpacked<T_Value, N_Out> slice(int32_t loIdx) const {
        VlUnpacked<T_Value, N_Out> out;
        for (std::size_t i = 0; i < N_Out; ++i) out.m_storage[i] = m_storage[loIdx + i];
        return out;
    }

    void fill(const T_Value& value) {
        std::fill(std::begin(m_storage), std::end(m_storage), value);
    }

    // To fit C++14
    template <std::size_t N_CurrentDimension = 0, typename U = T_Value>
    int find_length(int dimension, std::false_type) const {
        return size();
    }

    template <std::size_t N_CurrentDimension = 0, typename U = T_Value>
    int find_length(int dimension, std::true_type) const {
        if (dimension == N_CurrentDimension) return size();
        return m_storage[0].template find_length<N_CurrentDimension + 1>(dimension);
    }

    template <std::size_t N_CurrentDimension = 0>
    int find_length(int dimension) const {
        return find_length<N_CurrentDimension>(dimension, std::is_class<T_Value>{});
    }

    template <std::size_t N_CurrentDimension = 0, typename U = T_Value>
    auto& find_element(const std::vector<size_t>& indices, std::false_type) {
        return m_storage[indices[N_CurrentDimension]];
    }

    template <std::size_t N_CurrentDimension = 0, typename U = T_Value>
    auto& find_element(const std::vector<size_t>& indices, std::true_type) {
        return m_storage[indices[N_CurrentDimension]]
            .template find_element<N_CurrentDimension + 1>(indices);
    }

    template <std::size_t N_CurrentDimension = 0>
    auto& find_element(const std::vector<size_t>& indices) {
        return find_element<N_CurrentDimension>(indices, std::is_class<T_Value>{});
    }

    T_Value& operator[](size_t index) { return m_storage[index]; }
    constexpr const T_Value& operator[](size_t index) const { return m_storage[index]; }

    // *this != that, which might be used for change detection/trigger computation, but avoid
    // operator overloading in VlUnpacked for safety in other contexts.
    bool neq(const VlUnpacked<T_Value, N_Depth>& that) const { return neq(*this, that); }
    // Similar to 'neq' above, *this = that used for change detection
    void assign(const VlUnpacked<T_Value, N_Depth>& that) { *this = that; }
    bool operator==(const VlUnpacked<T_Value, N_Depth>& that) const { return !neq(that); }
    bool operator!=(const VlUnpacked<T_Value, N_Depth>& that) const { return neq(that); }
    // interface to C style arrays (used in ports), see issue #5125
    bool neq(const T_Value that[N_Depth]) const { return neq(*this, that); }
    void assign(const T_Value that[N_Depth]) { std::copy_n(that, N_Depth, m_storage); }
    void operator=(const T_Value that[N_Depth]) { assign(that); }
    bool operator<(const VlUnpacked<T_Value, N_Depth>& that) const {
        for (int index = 0; index < N_Depth; ++index) {
            if (m_storage[index] < that.m_storage[index]) return true;
        }
        return false;
    }

    // inside (set membership operator)
    bool inside(const T_Value& value) const {
        return std::find(std::begin(m_storage), std::end(m_storage), value) != std::end(m_storage);
    }

    void sort() { std::sort(std::begin(m_storage), std::end(m_storage)); }
    template <typename T_Func>
    void sort(T_Func with_func) {
        // with_func returns arbitrary type to use for the sort comparison
        std::sort(std::begin(m_storage), std::end(m_storage),
                  [=](const T_Value& a, const T_Value& b) {
                      // index number is meaningless with sort, as it changes
                      return with_func(0, a) < with_func(0, b);
                  });
    }
    // std::rbegin/std::rend not available until C++14
    void rsort() {
        std::sort(std::begin(m_storage), std::end(m_storage), std::greater<T_Value>());
    }
    template <typename T_Func>
    void rsort(T_Func with_func) {
        // with_func returns arbitrary type to use for the sort comparison
        // std::rbegin/std::rend not available until C++14, so using > below
        std::sort(std::begin(m_storage), std::end(m_storage),
                  [=](const T_Value& a, const T_Value& b) {
                      // index number is meaningless with sort, as it changes
                      return with_func(0, a) > with_func(0, b);
                  });
    }
    void reverse() { std::reverse(std::begin(m_storage), std::end(m_storage)); }
    void shuffle() { std::shuffle(std::begin(m_storage), std::end(m_storage), VlURNG{}); }
    VlQueue<T_Value> unique() const {
        VlQueue<T_Value> out;
        std::set<T_Value> saw;
        for (const auto& i : m_storage) {
            const auto it = saw.find(i);
            if (it == saw.end()) {
                saw.insert(it, i);
                out.push_back(i);
            }
        }
        return out;
    }
    template <typename T_Func>
    VlQueue<T_Value> unique(T_Func with_func) const {
        VlQueue<T_Value> out;
        std::set<T_Value> saw;
        for (const auto& i : m_storage) {
            const auto i_mapped = with_func(0, i);
            const auto it = saw.find(i_mapped);
            if (it == saw.end()) {
                saw.insert(it, i_mapped);
                out.push_back(i);
            }
        }
        return out;
    }
    VlQueue<T_Key> unique_index() const {
        VlQueue<T_Key> out;
        IData index = 0;
        std::set<T_Value> saw;
        for (const auto& i : m_storage) {
            const auto it = saw.find(i);
            if (it == saw.end()) {
                saw.insert(it, i);
                out.push_back(index);
            }
            ++index;
        }
        return out;
    }
    template <typename T_Func>
    VlQueue<T_Key> unique_index(T_Func with_func) const {
        VlQueue<T_Key> out;
        IData index = 0;
        std::set<T_Value> saw;
        for (const auto& i : m_storage) {
            const auto i_mapped = with_func(index, i);
            auto it = saw.find(i_mapped);
            if (it == saw.end()) {
                saw.insert(it, i_mapped);
                out.push_back(index);
            }
            ++index;
        }
        return out;
    }
    template <typename T_Func>
    VlQueue<T_Value> find(T_Func with_func) const {
        VlQueue<T_Value> out;
        IData index = 0;
        for (const auto& i : m_storage) {
            if (with_func(index, i)) out.push_back(i);
            ++index;
        }
        return out;
    }
    template <typename T_Func>
    VlQueue<T_Key> find_index(T_Func with_func) const {
        VlQueue<T_Key> out;
        IData index = 0;
        for (const auto& i : m_storage) {
            if (with_func(index, i)) out.push_back(index);
            ++index;
        }
        return out;
    }
    template <typename T_Func>
    VlQueue<T_Value> find_first(T_Func with_func) const {
        // Can't use std::find_if as need index number
        IData index = 0;
        for (const auto& i : m_storage) {
            if (with_func(index, i)) return VlQueue<T_Value>::consV(i);
            ++index;
        }
        return VlQueue<T_Value>{};
    }
    template <typename T_Func>
    VlQueue<T_Key> find_first_index(T_Func with_func) const {
        IData index = 0;
        for (const auto& i : m_storage) {
            if (with_func(index, i)) return VlQueue<IData>::consV(index);
            ++index;
        }
        return VlQueue<T_Key>{};
    }
    template <typename T_Func>
    VlQueue<T_Value> find_last(T_Func with_func) const {
        for (int i = N_Depth - 1; i >= 0; i--) {
            if (with_func(i, m_storage[i])) return VlQueue<T_Value>::consV(m_storage[i]);
        }
        return VlQueue<T_Value>{};
    }
    template <typename T_Func>
    VlQueue<T_Key> find_last_index(T_Func with_func) const {
        for (int i = N_Depth - 1; i >= 0; i--) {
            if (with_func(i, m_storage[i])) return VlQueue<IData>::consV(i);
        }
        return VlQueue<T_Key>{};
    }
    // Map method (IEEE 1800-2023 7.12.5)
    template <typename T_Func>
    VlQueue<WithFuncReturnType<T_Func>> map(T_Func with_func) const {
        VlQueue<WithFuncReturnType<T_Func>> out;
        for (IData i = 0; i < N_Depth; ++i) out.push_back(with_func(i, m_storage[i]));
        return out;
    }

    // Reduction operators
    VlQueue<T_Value> min() const {
        const auto it = std::min_element(std::begin(m_storage), std::end(m_storage));
        return VlQueue<T_Value>::consV(*it);
    }
    template <typename T_Func>
    VlQueue<T_Value> min(T_Func with_func) const {
        const auto it = std::min_element(std::begin(m_storage), std::end(m_storage),
                                         [&with_func](const IData& a, const IData& b) {
                                             return with_func(0, a) < with_func(0, b);
                                         });
        return VlQueue<T_Value>::consV(*it);
    }
    VlQueue<T_Value> max() const {
        const auto it = std::max_element(std::begin(m_storage), std::end(m_storage));
        return VlQueue<T_Value>::consV(*it);
    }
    template <typename T_Func>
    VlQueue<T_Value> max(T_Func with_func) const {
        const auto it = std::max_element(std::begin(m_storage), std::end(m_storage),
                                         [&with_func](const IData& a, const IData& b) {
                                             return with_func(0, a) < with_func(0, b);
                                         });
        return VlQueue<T_Value>::consV(*it);
    }

    T_Value r_sum() const {
        T_Value out(0);  // Type must have assignment operator
        for (const auto& i : m_storage) out += i;
        return out;
    }
    template <typename T_Func>
    WithFuncReturnType<T_Func> r_sum(T_Func with_func) const {
        WithFuncReturnType<T_Func> out
            = WithFuncReturnType<T_Func>(0);  // Type must have assignment operator
        for (const auto& i : m_storage) out += with_func(0, i);
        return out;
    }
    T_Value r_product() const {
        T_Value out = T_Value(1);
        for (const auto& i : m_storage) out *= i;
        return out;
    }
    template <typename T_Func>
    WithFuncReturnType<T_Func> r_product(T_Func with_func) const {
        WithFuncReturnType<T_Func> out = WithFuncReturnType<T_Func>(1);
        for (const auto& i : m_storage) out *= with_func(0, i);
        return out;
    }
    T_Value r_and() const {
        if (m_storage.empty()) return T_Value(0);  // The big three do it this way
        T_Value out = ~T_Value(0);
        for (const auto& i : m_storage) out &= i;
        return out;
    }
    template <typename T_Func>
    WithFuncReturnType<T_Func> r_and(T_Func with_func) const {
        WithFuncReturnType<T_Func> out = ~WithFuncReturnType<T_Func>(0);
        for (const auto& i : m_storage) out &= with_func(0, i);
        return out;
    }
    T_Value r_or() const {
        T_Value out = T_Value(0);
        for (const auto& i : m_storage) out |= i;
        return out;
    }
    template <typename T_Func>
    WithFuncReturnType<T_Func> r_or(T_Func with_func) const {
        WithFuncReturnType<T_Func> out = WithFuncReturnType<T_Func>(0);
        for (const auto& i : m_storage) out |= with_func(0, i);
        return out;
    }
    T_Value r_xor() const {
        T_Value out = T_Value(0);
        for (const auto& i : m_storage) out ^= i;
        return out;
    }
    template <typename T_Func>
    WithFuncReturnType<T_Func> r_xor(T_Func with_func) const {
        WithFuncReturnType<T_Func> out = WithFuncReturnType<T_Func>(0);
        for (const auto& i : m_storage) out ^= with_func(0, i);
        return out;
    }

    // Dumping. Verilog: str = $sformatf("%p", assoc)
    std::string to_string() const {
        std::string out = "'{";
        std::string comma;
        for (int i = 0; i < N_Depth; ++i) {
            out += comma + VL_TO_STRING(m_storage[i]);
            comma = ", ";
        }
        return out + "}";
    }

private:
    template <typename T_Val, std::size_t N_Dep>
    static bool neq(const VlUnpacked<T_Val, N_Dep>& a, const VlUnpacked<T_Val, N_Dep>& b) {
        for (size_t i = 0; i < N_Dep; ++i) {
            // Recursive 'neq', in case T_Val is also a VlUnpacked<_, _>
            if (neq(a.m_storage[i], b.m_storage[i])) return true;
        }
        return false;
    }

    template <typename T_Val, std::size_t N_Dep>
    static bool neq(const VlUnpacked<T_Val, N_Dep>& a, const T_Val b[N_Dep]) {
        for (size_t i = 0; i < N_Dep; ++i) {
            // Recursive 'neq', in case T_Val is also a VlUnpacked<_, _>
            if (neq(a.m_storage[i], b[i])) return true;
        }
        return false;
    }

    template <typename T_Other>  //
    static bool neq(const T_Other& a, const T_Other& b) {
        // Base case (T_Other is not VlUnpacked<_, _>), fall back on !=
        return a != b;
    }
};
// Trait to detect VlUnpacked types
template <typename T>
struct IsVlUnpacked : std::false_type {};
template <typename T, std::size_t N>
struct IsVlUnpacked<VlUnpacked<T, N>> : std::true_type {};

template <typename T_Value, std::size_t N_Depth>
std::string VL_TO_STRING(const VlUnpacked<T_Value, N_Depth>& obj) {
    return obj.to_string();
}

template <typename T_Value, std::size_t N_Depth>
struct VlContainsCustomStruct<VlUnpacked<T_Value, N_Depth>> : VlContainsCustomStruct<T_Value> {};

template <typename T_Value>
template <typename T_UnpackedValue, std::size_t N_UnpackedDepth>
VL_ATTR_NOINLINE void VlQueue<T_Value, 0>::renew_copy(
    size_t size, const VlUnpacked<T_UnpackedValue, N_UnpackedDepth>& rhs) {
    clear();
    if (size == 0) return;
    resize(size);
    for (size_t i = 0; i < std::min(size, N_UnpackedDepth); ++i) { element(i) = rhs.m_storage[i]; }
}

//===================================================================
// Helper to apply the given indices to a target expression

template <size_t N_Curr, size_t N_Rank, typename T_Target>
struct VlApplyIndices final {
    VL_ATTR_ALWINLINE
    static auto& apply(T_Target& target, const size_t* indicesp) {
        return VlApplyIndices<N_Curr + 1, N_Rank, decltype(target[indicesp[N_Curr]])>::apply(
            target[indicesp[N_Curr]], indicesp);
    }
};

template <size_t N_Rank, typename T_Target>
struct VlApplyIndices<N_Rank, N_Rank, T_Target> final {
    VL_ATTR_ALWINLINE
    static T_Target& apply(T_Target& target, const size_t*) { return target; }
};

//===================================================================
// Commit queue for NBAs - currently only for unpacked arrays
//
// This data-structure is used to handle non-blocking assignments
// that might execute a variable number of times in a single
// evaluation. It has 2 operations:
// - 'enqueue' will add an update to the queue
// - 'commit' will apply all enqueued updates to the target variable,
//   in the order they were enqueued. This ensures the last NBA
//   takes effect as it is expected.
// There are 2 specializations of this class below:
// - A version when a partial element update is not required,
//   e.g, to handle:
//      logic [31:0] array[N];
//      for (int i = 0 ; i < N ; ++i) array[i] <= x;
//   Here 'enqueue' takes the RHS ('x'), and the array indices ('i')
//   as arguments.
// - A different version when a partial element update is required,
//   e.g. for:
//      logic [31:0] array[N];
//      for (int i = 0 ; i < N ; ++i) array[i][3:1] <= y;
//   Here 'enqueue' takes one additional argument, which is a bitmask
//   derived from the bit selects (_[3:1]), which masks the bits that
//   need to be updated, and additionally the RHS is widened to a full
//   element size, with the bits inserted into the masked region.
template <typename T_Target,  // Type of the variable this commit queue updates
          bool Partial,  // Whether partial element updates are necessary
          // The following we could figure out from 'T_Target using type traits, but passing
          // explicitly to avoid template expansion, as Verilator already knows them
          typename T_Element,  // Non-array leaf element type of T_Target array
          std::size_t N_Rank  // Rank of T_Target (i.e.: how many dimensions it has)
          >
class VlNBACommitQueue;

// Specialization for whole element updates only
template <typename T_Target, typename T_Element, std::size_t N_Rank>
class VlNBACommitQueue<T_Target, /* Partial: */ false, T_Element, N_Rank> final {
    // TYPES
    struct Entry final {
        T_Element value;
        size_t indices[N_Rank];
    };

    // STATE
    std::vector<Entry> m_pending;  // Pending updates, in program order

public:
    // CONSTRUCTOR
    VlNBACommitQueue() = default;
    VL_UNCOPYABLE(VlNBACommitQueue);

    // METHODS
    template <typename... T_Args>
    void enqueue(const T_Element& value, T_Args... indices) {
        m_pending.emplace_back(Entry{value, {indices...}});
    }

    // Note: T_Commit might be different from T_Target. Specifically, when the signal is a
    // top-level IO port, T_Commit will be a native C array, while T_Target, will be a VlUnpacked
    template <typename T_Commit>
    void commit(T_Commit& target) {
        if (m_pending.empty()) return;
        for (const Entry& entry : m_pending) {
            VlApplyIndices<0, N_Rank, T_Commit>::apply(target, entry.indices) = entry.value;
        }
        m_pending.clear();
    }
};

// With partial element updates
template <typename T_Target, typename T_Element, std::size_t N_Rank>
class VlNBACommitQueue<T_Target, /* Partial: */ true, T_Element, N_Rank> final {
    // TYPES
    struct Entry final {
        T_Element value;
        T_Element mask;
        size_t indices[N_Rank];
    };

    // STATE
    std::vector<Entry> m_pending;  // Pending updates, in program order

    // STATIC METHODS

    // Binary & | ~ for elements to use for masking in partial updates. Sorry for the templates.
    template <typename T>
    VL_ATTR_ALWINLINE static typename std::enable_if<!VlIsVlWide<T>::value, T>::type
    bAnd(const T& a, const T& b) {
        return a & b;
    }

    template <typename T>
    VL_ATTR_ALWINLINE static typename std::enable_if<VlIsVlWide<T>::value, T>::type
    bAnd(const T& a, const T& b) {
        T result;
        for (size_t i = 0; i < T::Words; ++i) {
            result.m_storage[i] = a.m_storage[i] & b.m_storage[i];
        }
        return result;
    }

    template <typename T>
    VL_ATTR_ALWINLINE static typename std::enable_if<!VlIsVlWide<T>::value, T>::type
    bOr(const T& a, const T& b) {
        return a | b;
    }

    template <typename T>
    VL_ATTR_ALWINLINE static typename std::enable_if<VlIsVlWide<T>::value, T>::type  //
    bOr(const T& a, const T& b) {
        T result;
        for (size_t i = 0; i < T::Words; ++i) {
            result.m_storage[i] = a.m_storage[i] | b.m_storage[i];
        }
        return result;
    }

    template <typename T>
    VL_ATTR_ALWINLINE static typename std::enable_if<!VlIsVlWide<T>::value, T>::type
    bNot(const T& a) {
        return ~a;
    }

    template <typename T>
    VL_ATTR_ALWINLINE static typename std::enable_if<VlIsVlWide<T>::value, T>::type
    bNot(const T& a) {
        T result;
        for (size_t i = 0; i < T::Words; ++i) result.m_storage[i] = ~a.m_storage[i];
        return result;
    }

public:
    // CONSTRUCTOR
    VlNBACommitQueue() = default;
    VL_UNCOPYABLE(VlNBACommitQueue);

    // METHODS
    template <typename... T_Args>
    void enqueue(const T_Element& value, const T_Element& mask, T_Args... indices) {
        m_pending.emplace_back(Entry{value, mask, {indices...}});
    }

    // Note: T_Commit might be different from T_Target. Specifically, when the signal is a
    // top-level IO port, T_Commit will be a native C array, while T_Target, will be a VlUnpacked
    template <typename T_Commit>
    void commit(T_Commit& target) {
        if (m_pending.empty()) return;
        for (const Entry& entry : m_pending) {  //
            auto& ref = VlApplyIndices<0, N_Rank, T_Commit>::apply(target, entry.indices);
            // Maybe inefficient, but it works for now ...
            const auto oldValue = ref;
            ref = bOr(bAnd(entry.value, entry.mask), bAnd(oldValue, bNot(entry.mask)));
        }
        m_pending.clear();
    }
};

//===================================================================
// Object that VlDeleter is capable of deleting

class VlDeletable VL_NOT_FINAL {
public:
    VlDeletable() = default;
    virtual ~VlDeletable() = default;
};

//===================================================================
// Class providing delayed deletion of garbage objects. Objects get deleted only when 'deleteAll()'
// is called, or the deleter itself is destroyed.

class VlDeleter final {
    // MEMBERS
    // Queue of new objects that should be deleted
    std::vector<VlDeletable*> m_newGarbage VL_GUARDED_BY(m_mutex);
    // Queue of objects currently being deleted (only for deleteAll())
    std::vector<VlDeletable*> m_deleteNow VL_GUARDED_BY(m_deleteMutex);
    mutable VerilatedMutex m_mutex;  // Mutex protecting the 'new garbage' queue
    mutable VerilatedMutex m_deleteMutex;  // Mutex protecting the delete queue

public:
    // CONSTRUCTOR
    VlDeleter() = default;
    ~VlDeleter() { deleteAll(); }

private:
    VL_UNCOPYABLE(VlDeleter);

public:
    // METHODS
    // Adds a new object to the 'new garbage' queue.
    void put(VlDeletable* const objp) VL_MT_SAFE {
        const VerilatedLockGuard lock{m_mutex};
        m_newGarbage.push_back(objp);
    }

    // Deletes all queued garbage objects.
    void deleteAll() VL_EXCLUDES(m_mutex) VL_EXCLUDES(m_deleteMutex) VL_MT_SAFE;
};

//===================================================================
// Base class for all verilated classes. Includes a reference counter, and a pointer to the deleter
// object that should destroy it after the counter reaches 0. This allows for easy construction of
// VlClassRefs from 'this'.

class VlClass VL_NOT_FINAL : public VlDeletable {
    // TYPES
    template <typename T_Class>
    friend class VlClassRef;  // Needed for access to the ref counter and deleter

    // MEMBERS
    std::atomic<size_t> m_counter{1};  // Reference count for this object
    VlDeleter* m_deleterp = nullptr;  // The deleter that will delete this object

    // METHODS
    // Atomically increments the reference counter
    void refCountInc() VL_MT_SAFE {
        VL_DEBUG_IFDEF(assert(m_counter););  // If zero, we might have already deleted
        ++m_counter;
    }
    // Atomically decrements the reference counter. Assuming VlClassRef semantics are sound, it
    // should never get called at m_counter == 0.
    void refCountDec() VL_MT_SAFE {
        if (!--m_counter) m_deleterp->put(this);
    }

public:
    // CONSTRUCTORS
    VlClass() {}
    VlClass(const VlClass& /*copied*/) {}
    ~VlClass() override = default;
    // Polymorphic shallow clone. Overridden in each generated concrete class.
    virtual VlClass* clone() const { return nullptr; }
    // METHODS
    size_t refCount() const VL_MT_SAFE { return m_counter; }
    virtual const char* typeName() const { return "VlClass"; }
    virtual std::string to_string() const { return ""; }
};

//===================================================================
// Represents the null pointer. Used for:
// * setting VlClassRef to null instead of via nullptr_t, to prevent the implicit conversion of 0
//   to nullptr,
// * comparing interface pointers to null.

struct VlNull final {
    operator bool() const { return false; }
    template <class T>
    operator T*() const {
        return nullptr;
    }
    template <class T>
    bool operator==(T* rhs) const {
        return !rhs;
    }
    template <class T>
    bool operator==(const T* rhs) const {
        return !rhs;
    }
};

template <class T>
inline bool operator==(T* lhs, VlNull) {
    return !lhs;
}
template <class T>
inline bool operator==(const T* lhs, VlNull) {
    return !lhs;
}

//===================================================================
// Verilog class reference container
// There are no multithreaded locks on this; the base variable must
// be protected by other means

template <typename T_Class>
class VlClassRef final {
private:
    // MEMBERS
    T_Class* m_objp = nullptr;  // Object pointed to

    // METHODS
    // Increase reference counter with null check
    void refCountInc() const VL_MT_SAFE {
        if (m_objp) m_objp->refCountInc();
    }
    // Decrease reference counter with null check
    void refCountDec() const VL_MT_SAFE {
        if (m_objp) m_objp->refCountDec();
    }

public:
    // CONSTRUCTORS
    VlClassRef() = default;
    // Init with nullptr
    // cppcheck-suppress noExplicitConstructor
    VlClassRef(VlNull) {};
    template <typename... T_Args>
    VlClassRef(VlDeleter& deleter, T_Args&&... args)
        : m_objp{new T_Class} {
        // Instantly init the object to presevrve RAII
        m_objp->init(std::forward<T_Args>(args)...);
        m_objp->m_deleterp = &deleter;
    }
    VlClassRef(VlDeleter& deleter, T_Class&& args)
        // Move constructor
        : m_objp{new T_Class{std::forward<T_Class>(args)}} {
        m_objp->m_deleterp = &deleter;
    }
    VlClassRef(VlDeleter& deleter, const T_Class& args)
        // Copy constructor
        : m_objp{new T_Class{args}} {
        m_objp->m_deleterp = &deleter;
    }
    VlClassRef(VlDeleter& deleter, T_Class& args)
        // Copy constructor - this is required since if `T_Class&`
        // will be provided a compiler will match it to the constructor
        // with variadic template instead of `T_Class&&`
        : m_objp{new T_Class{args}} {
        m_objp->m_deleterp = &deleter;
    }
    // Explicit to avoid implicit conversion from 0
    explicit VlClassRef(T_Class* objp)
        : m_objp{objp} {
        refCountInc();
    }
    // cppcheck-suppress noExplicitConstructor
    VlClassRef(const VlClassRef& copied)
        : m_objp{copied.m_objp} {
        refCountInc();
    }
    // cppcheck-suppress noExplicitConstructor
    VlClassRef(VlClassRef&& moved)
        : m_objp{std::exchange(moved.m_objp, nullptr)} {}
    // cppcheck-suppress noExplicitConstructor
    template <typename T_OtherClass>
    VlClassRef(const VlClassRef<T_OtherClass>& copied)
        : m_objp{copied.operator->()} {
        refCountInc();
    }
    // cppcheck-suppress noExplicitConstructor
    template <typename T_OtherClass>
    VlClassRef(VlClassRef<T_OtherClass>&& moved)
        : m_objp{moved.release()} {}
    ~VlClassRef() { refCountDec(); }

    // METHODS
    T_Class* release() noexcept { return std::exchange(m_objp, nullptr); }
    // Copy and move assignments
    VlClassRef& operator=(const VlClassRef& copied) {
        if (m_objp == copied.m_objp) return *this;
        refCountDec();
        m_objp = copied.m_objp;
        refCountInc();
        return *this;
    }
    VlClassRef& operator=(VlClassRef&& moved) {
        if (m_objp == moved.m_objp) return *this;
        refCountDec();
        m_objp = std::exchange(moved.m_objp, nullptr);
        return *this;
    }
    template <typename T_OtherClass>
    VlClassRef& operator=(const VlClassRef<T_OtherClass>& copied) {
        if (m_objp == copied.operator->()) return *this;
        refCountDec();
        m_objp = copied.operator->();
        refCountInc();
        return *this;
    }
    template <typename T_OtherClass>
    VlClassRef& operator=(VlClassRef<T_OtherClass>&& moved) {
        if (m_objp == moved.operator->()) return *this;
        refCountDec();
        m_objp = moved.release();
        return *this;
    }
    // Assign with nullptr
    VlClassRef& operator=(VlNull) {
        refCountDec();
        m_objp = nullptr;
        return *this;
    }
    // Dynamic caster
    template <typename T_OtherClass>
    VlClassRef<T_OtherClass> dynamicCast() const {
        return VlClassRef<T_OtherClass>{dynamic_cast<T_OtherClass*>(m_objp)};
    }
    // Polymorphic shallow clone (IEEE 1800-2023 8.7: new <handle> preserves runtime type)
    VlClassRef clone(VlDeleter& deleter) const {
        VlClass* clonedp = m_objp->clone();
        if (VL_UNLIKELY(!clonedp)) return {};
        clonedp->m_deleterp = &deleter;
        VlClassRef result;
        result.m_objp = dynamic_cast<T_Class*>(clonedp);
        return result;
    }
    // Dereference operators
    T_Class& operator*() const { return *m_objp; }
    T_Class* operator->() const { return m_objp; }
    // For 'if (ptr)...'
    operator bool() const { return m_objp; }
    // In SV A == B iff both are handles to the same object (IEEE 1800-2023 8.4)
    template <typename T_OtherClass>
    bool operator==(const VlClassRef<T_OtherClass>& rhs) const {
        return m_objp == rhs.operator->();
    };
    template <typename T_OtherClass>
    bool operator!=(const VlClassRef<T_OtherClass>& rhs) const {
        return m_objp != rhs.operator->();
    };
    template <typename T_OtherClass>
    bool operator<(const VlClassRef<T_OtherClass>& rhs) const {
        return m_objp < rhs.operator->();
    };
};

template <typename T_Lhs, typename T_Out>
inline bool VL_CAST_DYNAMIC(VlClassRef<T_Lhs> in, VlClassRef<T_Out>& outr) {
    if (!in) {
        outr = VlNull{};
        return true;
    }
    VlClassRef<T_Out> casted = in.template dynamicCast<T_Out>();
    if (VL_LIKELY(casted)) {
        outr = casted;
        return true;
    }
    return false;
}

template <typename T_Lhs>
inline bool VL_CAST_DYNAMIC(VlNull, VlClassRef<T_Lhs>& outr) {
    outr = VlNull{};
    return true;
}

// For printing class references under a container, several choices:
// 1. Dump recursively the pointed-to object.  Can be huge.  Might be circular.
// 2. Print object type and pointer as C pointer.  Astable when rerun.
// 3. Print object type and pointer as an incrementing number.  Needs num storage.
// 4. Print object type alone.  Avoids above issues.
template <typename T_Lhs>
inline std::string VL_TO_STRING(const VlClassRef<T_Lhs>& obj) {
    return obj ? obj->typeName() : "null";
}
// Entry point for string conversion (called from not under a container);
// dereference VlClassRef objects to print members
template <typename T_Lhs>  // Default if no specialization
inline std::string VL_TO_STRING_DEREF(T_Lhs obj) {
    return VL_TO_STRING(obj);
}
template <typename T_Lhs>  // Specialization
inline std::string VL_TO_STRING_DEREF(const VlClassRef<T_Lhs>& obj) {
    return obj ? obj->to_string() : "null";
}
template <typename T_Lhs>  // Specialization
inline std::string VL_TO_STRING_DEREF(VlClassRef<T_Lhs>& obj) {
    return obj ? obj->to_string() : "null";
}

//=============================================================================
// VlSampleQueue stores samples for input clockvars in clocking blocks. At a clocking event,
// samples from this queue should be written to the correct input clockvar.

template <typename T_Sampled>
class VlSampleQueue final {
    // TYPES
    // Type representing a single value sample at a point in time
    struct VlSample final {
        uint64_t m_timestamp;  // Timestamp at which the value was sampled
        T_Sampled m_value;  // The sampled value
    };

    // MEMBERS
    std::deque<VlSample> m_queue;  // Queue of samples with timestamps

public:
    // METHODS
    // Push a new sample with the given timestamp to the end of the queue
    void push(uint64_t time, const T_Sampled& value) { m_queue.push_back({time, value}); }
    // Get the latest sample with its timestamp less than or equal to the given skew
    void pop(uint64_t time, uint64_t skew, T_Sampled& value) {
        if (time < skew) return;
        // Find the last element not greater than (time - skew). Do a binary search, as the queue
        // should be ordered.
        auto it = std::lower_bound(m_queue.rbegin(), m_queue.rend(), VlSample{time - skew, {}},
                                   [](const VlSample& sample, const VlSample& skewed) {
                                       return sample.m_timestamp > skewed.m_timestamp;
                                   });
        if (it != m_queue.rend()) {
            value = it->m_value;
            m_queue.erase(m_queue.begin(), it.base());
        }
    }
};

//======================================================================

#define VL_NEW(Class, ...) \
    VlClassRef<Class> { vlSymsp->__Vm_deleter, __VA_ARGS__ }

#define VL_KEEP_THIS \
    VlClassRef<std::remove_pointer<decltype(this)>::type> __Vthisref { this }

template <typename T>  // T typically of type VlClassRef<x>
inline T VL_NULL_CHECK(T t, const char* filename, int linenum) {
    if (VL_UNLIKELY(!t)) Verilated::nullPointerError(filename, linenum);
    return t;
}

//======================================================================

template <typename T_Value>
template <typename T_Subclass>
VL_ATTR_NOINLINE VlQueue<T_Value, 0>::VlQueue(
    const VlQueue<VlClassRef<T_Subclass>, 0>& that)
    : m_defaultValue{that.atDefault()} {
    for (int i = 0; i < that.size(); ++i) push_back(that.at(i));
}

template <typename T_Value>
template <typename T_Subclass>
VL_ATTR_NOINLINE VlQueue<T_Value, 0>::VlQueue(VlQueue<VlClassRef<T_Subclass>, 0>&& that)
    : m_defaultValue{std::move(that.atDefault())} {
    for (int i = 0; i < that.size(); ++i) {
        T_Value moved{std::move(that.atWrite(i))};
        push_back(moved);
    }
}

#endif  // Guard
