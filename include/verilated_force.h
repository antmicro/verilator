// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
//
// Code available from: https://verilator.org
//
// Copyright 2026-2026 by Wilson Snyder. This program is free software; you can
// redistribute it and/or modify it under the terms of either the GNU
// Lesser General Public License Version 3 or the Perl Artistic License
// Version 2.0.
// SPDX-FileCopyrightText: 2026 Wilson Snyder
// SPDX-License-Identifier: LGPL-3.0-only OR Artistic-2.0
//
//*************************************************************************
///
/// \file
/// \brief Verilator: Runtime support for force/release statements
///
/// This file provides runtime data structures for efficient dynamic
/// resolution of force/release statements. A sorted list of active
/// forces is maintained that can be efficiently queried and modified
/// at runtime.
///
//*************************************************************************

#ifndef VERILATOR_VERILATED_FORCE_H_
#define VERILATOR_VERILATED_FORCE_H_

#include "verilatedos.h"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <type_traits>
#include <vector>

//=============================================================================
// VlForceEntry - Represents an active force on a bit range
//
// Each entry holds:
// - lsb/msb: The bit range being forced (inclusive)
// - rhsDatap: Pointer to the force RHS shadow storage
// - rhsLsb: Destination index that maps to RHS index 0
//
// Entries are kept sorted by lsb. Ranges never overlap.

struct VlForceEntry final {
    int m_lsb;  // Inclusive lower bit
    int m_msb;  // Inclusive upper bit
    int m_rhsLsb;  // Destination index that maps to RHS index 0
    const void* m_rhsDatap;  // Pointer to RHS storage

    bool operator<(const VlForceEntry& other) const { return m_lsb < other.m_lsb; }
};

//=============================================================================
// VlForceVec - Vector of active force entries for a signal
//
// This class maintains a sorted vector of non-overlapping force entries.
// When a new force is added, it removes or trims existing entries that
// overlap with the new range.
//
// The generated code will:
// 1. Use addForce/release to update the active forces
// 2. Call a generated read function that iterates entries and evaluates RHS

class VlForceVec final {
private:
    std::vector<VlForceEntry> m_entries;  // Sorted by lsb, non-overlapping

    void trimEntries(int lsb, int msb, std::vector<VlForceEntry>& newEntries) const {
        for (const auto& entry : m_entries) {
            if (entry.m_msb < lsb || entry.m_lsb > msb) {
                newEntries.push_back(entry);
                continue;
            }
            if (entry.m_lsb < lsb) {
                newEntries.push_back({entry.m_lsb, lsb - 1, entry.m_rhsLsb, entry.m_rhsDatap});
            }
            if (entry.m_msb > msb) {
                newEntries.push_back({msb + 1, entry.m_msb, entry.m_rhsLsb, entry.m_rhsDatap});
            }
        }
    }

public:
    VlForceVec() = default;

    // Check if there are any active forces
    bool empty() const { return m_entries.empty(); }

    // Get entries for iteration
    const std::vector<VlForceEntry>& entries() const { return m_entries; }

    // Add a new force on range [lsb:msb] with a pointer to RHS data.
    // This will remove or trim any overlapping existing forces
    void addForce(int lsb, int msb, const void* rhsDatap, int rhsLsb) {
        assert(lsb <= msb);
        assert(rhsDatap);
        assert(rhsLsb <= lsb);

        std::vector<VlForceEntry> newEntries;
        newEntries.reserve(m_entries.size() + 2);  // May split one entry
        trimEntries(lsb, msb, newEntries);
        newEntries.push_back({lsb, msb, rhsLsb, rhsDatap});

        // Sort by lsb to maintain order
        std::sort(newEntries.begin(), newEntries.end());

        m_entries = std::move(newEntries);
    }

    // Release (remove force from) range [lsb:msb]
    void release(int lsb, int msb) {
        if (lsb > msb) return;

        std::vector<VlForceEntry> newEntries;
        newEntries.reserve(m_entries.size() + 1);  // May split one entry
        trimEntries(lsb, msb, newEntries);
        m_entries = std::move(newEntries);
    }
};

//=============================================================================
// VlForceRead - Helper functions to read a forced value
//
// These functions combine original value with forced values based on
// VlForceVec entries.
// This achieves O(k) complexity where k = number of active forces.

template <typename T>
struct VlForceIsBitwise final {
    static constexpr bool value
        = std::is_integral<T>::value || std::is_enum<T>::value || VlIsVlWide<T>::value;
};

template <typename T>
struct VlForceArrayIndexer final {
    static constexpr std::size_t size = 1;

    static T& elem(T& value, std::size_t) { return value; }
    static const T& elem(const T& value, std::size_t) { return value; }
};

template <typename T, std::size_t N>
struct VlForceArrayIndexer<VlUnpacked<T, N>> final {
    static constexpr std::size_t size = N * VlForceArrayIndexer<T>::size;

    static auto& elem(VlUnpacked<T, N>& array, std::size_t index) {
        constexpr std::size_t subSize = VlForceArrayIndexer<T>::size;
        return VlForceArrayIndexer<T>::elem(array[index / subSize], index % subSize);
    }
    static const auto& elem(const VlUnpacked<T, N>& array, std::size_t index) {
        constexpr std::size_t subSize = VlForceArrayIndexer<T>::size;
        return VlForceArrayIndexer<T>::elem(array[index / subSize], index % subSize);
    }
};

template <typename T, bool IsEnum = std::is_enum<T>::value>
struct VlForceStorageType final {
    using type = typename std::make_unsigned<T>::type;
};

template <typename T>
static inline typename VlForceStorageType<T>::type VlForceToStorage(T value) {
    return static_cast<typename VlForceStorageType<T>::type>(value);
}

template <typename T>
static inline T VlForceFromStorage(typename VlForceStorageType<T>::type value) {
    return static_cast<T>(value);
}

static inline QData VlForceExtractRhsChunk(const VlForceEntry& entry, int rhsLsb, int width) {
    assert(width > 0 && width <= 64);
    assert(rhsLsb >= 0);

    const QData mask
        = width >= 64 ? ~static_cast<QData>(0) : ((static_cast<QData>(1) << width) - 1);
    const int rhsWidth = entry.m_msb - entry.m_rhsLsb + 1;
    if (rhsWidth <= 64) {
        const QData rhsVal = static_cast<QData>(*static_cast<const QData*>(entry.m_rhsDatap));
        return (rhsVal >> rhsLsb) & mask;
    }

    const EData* const rhswp = static_cast<const EData*>(entry.m_rhsDatap);
    const int startWord = VL_BITWORD_E(rhsLsb);
    const int startBit = VL_BITBIT_E(rhsLsb);
    QData out = static_cast<QData>(rhswp[startWord]) >> startBit;
    int outBits = VL_EDATASIZE - startBit;
    if (outBits < width) {
        out |= static_cast<QData>(rhswp[startWord + 1]) << outBits;
        outBits += VL_EDATASIZE;
        if (outBits < width) { out |= static_cast<QData>(rhswp[startWord + 2]) << outBits; }
    }
    return out & mask;
}

template <typename T,
          typename std::enable_if<VlForceIsBitwise<T>::value && !VlIsVlWide<T>::value, int>::type
          = 0>
static inline T VlForceApplyEntry(T result, const VlForceEntry& entry) {
    using U = typename VlForceStorageType<T>::type;
    const int width = entry.m_msb - entry.m_lsb + 1;
    const int bits = static_cast<int>(sizeof(U) * 8);
    const int rhsLsb = entry.m_lsb - entry.m_rhsLsb;
    const QData rhsChunk = VlForceExtractRhsChunk(entry, rhsLsb, width);
    if (width >= bits) return VlForceFromStorage<T>(static_cast<U>(rhsChunk));
    const U lowMask = static_cast<U>((static_cast<QData>(1) << width) - 1);
    const U mask = static_cast<U>(lowMask << entry.m_lsb);
    const U rhsVal = static_cast<U>((static_cast<U>(rhsChunk) & lowMask) << entry.m_lsb);
    const U cur = VlForceToStorage(result);
    return VlForceFromStorage<T>(static_cast<U>((cur & ~mask) | rhsVal));
}

// For non-bitwise types, only one force is possible, lsb=msb=0
template <typename T, typename std::enable_if<!VlForceIsBitwise<T>::value, int>::type = 0>
static inline T VlForceApplyEntry(T /*result*/, const VlForceEntry& entry) {
    return *static_cast<const T*>(entry.m_rhsDatap);
}

template <std::size_t N_Words>
static inline VlWide<N_Words> VlForceApplyEntry(VlWide<N_Words> result,
                                                const VlForceEntry& entry) {
    EData* const reswp = result.data();
    const int lword = VL_BITWORD_E(entry.m_lsb);
    const int hword = VL_BITWORD_E(entry.m_msb);
    for (int word = lword; word <= hword; ++word) {
        const int wordLsb = word * VL_EDATASIZE;
        const int segLsb = std::max(entry.m_lsb, wordLsb);
        const int segMsb = std::min(entry.m_msb, wordLsb + VL_EDATASIZE - 1);
        const int segWidth = segMsb - segLsb + 1;
        const int bitOffset = segLsb - wordLsb;
        const int rhsLsb = segLsb - entry.m_rhsLsb;
        const EData mask = segWidth >= VL_EDATASIZE
                               ? ~static_cast<EData>(0)
                               : static_cast<EData>(VL_MASK_E(segWidth)) << bitOffset;
        const EData rhsBits = static_cast<EData>(VlForceExtractRhsChunk(entry, rhsLsb, segWidth))
                              << bitOffset;
        reswp[word] = (reswp[word] & ~mask) | (rhsBits & mask);
    }
    return result;
}

template <typename T>
static inline T VlForceRead(T origVal, const VlForceVec& forceVec) {
    if (forceVec.empty()) return origVal;

    T result = origVal;
    for (const auto& entry : forceVec.entries()) { result = VlForceApplyEntry(result, entry); }
    return result;
}

template <typename T, typename... TDeps>
static inline T VlForceRead(T origVal, const VlForceVec& forceVec, const TDeps&...) {
    return VlForceRead(origVal, forceVec);
}

template <typename T>
static inline T VlForceReadIndex(T origVal, const VlForceVec& forceVec, int index) {
    if (forceVec.empty()) return origVal;

    const auto& entries = forceVec.entries();
    // Binary search: find first entry with msb >= index
    const auto it = std::lower_bound(entries.begin(), entries.end(), index,
                                     [](const VlForceEntry& e, int idx) { return e.m_msb < idx; });
    if (it != entries.end() && it->m_lsb <= index) {
        const int rhsIndex = index - it->m_rhsLsb;
        const T* const rhsBasep = static_cast<const T*>(it->m_rhsDatap);
        return rhsBasep[rhsIndex];
    }
    return origVal;
}

template <typename T, typename... TDeps>
static inline T VlForceReadIndex(T origVal, const VlForceVec& forceVec, int index,
                                 const TDeps&...) {
    return VlForceReadIndex(origVal, forceVec, index);
}

template <typename T>
static inline T VlForceReadArray(const T& origVal, const VlForceVec& forceVec) {
    if (forceVec.empty()) return origVal;

    T result = origVal;
    using ElemRef = decltype(VlForceArrayIndexer<T>::elem(result, static_cast<std::size_t>(0)));
    using Elem = typename std::remove_reference<ElemRef>::type;
    const int total = static_cast<int>(VlForceArrayIndexer<T>::size);
    for (const auto& entry : forceVec.entries()) {
        const Elem* const rhsBasep = static_cast<const Elem*>(entry.m_rhsDatap);
        const int startIdx = std::max(0, entry.m_lsb);
        const int endIdx = std::min(total - 1, entry.m_msb);
        for (int idx = startIdx; idx <= endIdx; ++idx) {
            const int rhsIndex = idx - entry.m_rhsLsb;
            const std::size_t uidx = static_cast<std::size_t>(idx);
            VlForceArrayIndexer<T>::elem(result, uidx) = rhsBasep[rhsIndex];
        }
    }
    return result;
}

template <typename T, typename... TDeps>
static inline T VlForceReadArray(const T& origVal, const VlForceVec& forceVec, const TDeps&...) {
    return VlForceReadArray(origVal, forceVec);
}

template <typename T>
static inline IData VlForceDep(const T& val) {
    return static_cast<IData>(val);
}

template <std::size_t N_Words>
static inline IData VlForceDep(const VlWide<N_Words>& val) {
    return val[0];
}

#endif  // guard
