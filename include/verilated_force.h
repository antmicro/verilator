// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
//
// Code available from: https://verilator.org
//
// Copyright 2026-2026 by Wilson Snyder. This program is free software; you can
// redistribute it and/or modify it under the terms of either the GNU
// Lesser General Public License Version 3 or the Perl Artistic License
// Version 2.0.
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
// - forceId: An identifier for which force statement is active (used by generated code)
//
// Entries are kept sorted by lsb. Ranges never overlap.

struct VlForceEntry final {
    int m_lsb;  // Inclusive lower bit
    int m_msb;  // Inclusive upper bit
    int m_forceId;  // Identifier for this force (used to select RHS in generated code)

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
            if (entry.m_lsb < lsb) newEntries.push_back({entry.m_lsb, lsb - 1, entry.m_forceId});
            if (entry.m_msb > msb) newEntries.push_back({msb + 1, entry.m_msb, entry.m_forceId});
        }
    }

public:
    VlForceVec() = default;

    // Check if there are any active forces
    bool empty() const { return m_entries.empty(); }

    // Get entries for iteration
    const std::vector<VlForceEntry>& entries() const { return m_entries; }

    // Add a new force on range [lsb:msb] with given forceId
    // This will remove or trim any overlapping existing forces
    void addForce(int lsb, int msb, int forceId) {
        assert(lsb <= msb);

        std::vector<VlForceEntry> newEntries;
        newEntries.reserve(m_entries.size() + 2);  // May split one entry
        trimEntries(lsb, msb, newEntries);
        newEntries.push_back({lsb, msb, forceId});

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
// VlForceVec entries. The rhsArray contains the RHS values for each force ID.
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

template <typename T, typename std::enable_if<VlForceIsBitwise<T>::value, int>::type = 0>
static inline T VlForceApplyEntry(T result, T rhsVal, int lsb, int msb) {
    const int width = msb - lsb + 1;
    const int bits = static_cast<int>(sizeof(T) * 8);
    const T mask
        = (width >= bits) ? ~static_cast<T>(0) : ((static_cast<T>(1) << width) - 1) << lsb;
    return (result & ~mask) | (rhsVal & mask);
}

// For non-bitwise types, only one force is possible, lsb=msb=0
template <typename T, typename std::enable_if<!VlForceIsBitwise<T>::value, int>::type = 0>
static inline T VlForceApplyEntry(T /*result*/, T rhsVal, int /*lsb*/, int /*msb*/) {
    return rhsVal;
}

template <std::size_t N_Words>
static inline VlWide<N_Words> VlForceApplyEntry(VlWide<N_Words> result,
                                                const VlWide<N_Words>& rhsVal, int lsb, int msb) {
    const int lword = VL_BITWORD_E(lsb);
    const int hword = VL_BITWORD_E(msb);
    const int loffset = VL_BITBIT_E(lsb);
    const int hoffset = VL_BITBIT_E(msb);
    EData* const reswp = result.data();
    const EData* const rhswp = rhsVal.data();

    for (int word = lword; word <= hword; ++word) {
        EData mask = ~static_cast<EData>(0);
        if (word == lword) {
            const EData lmask = static_cast<EData>(VL_MASK_E(VL_EDATASIZE - loffset)) << loffset;
            mask &= lmask;
        }
        if (word == hword) mask &= static_cast<EData>(VL_MASK_E(hoffset + 1));
        reswp[word] = (reswp[word] & ~mask) | (rhswp[word] & mask);
    }
    return result;
}

template <typename T>
static inline T VlForceRead(T origVal, const VlForceVec& forceVec, const T* rhsArray) {
    if (forceVec.empty()) return origVal;

    T result = origVal;
    for (const auto& entry : forceVec.entries()) {
        result = VlForceApplyEntry(result, rhsArray[entry.m_forceId], entry.m_lsb, entry.m_msb);
    }
    return result;
}

template <typename T, typename TRhsArray>
static inline T VlForceReadIndex(T origVal, const VlForceVec& forceVec, const TRhsArray* rhsArray,
                                 int index) {
    if (forceVec.empty()) return origVal;

    const auto& entries = forceVec.entries();
    // Binary search: find first entry with msb >= index
    const auto it = std::lower_bound(entries.begin(), entries.end(), index,
                                     [](const VlForceEntry& e, int idx) { return e.m_msb < idx; });
    if (it != entries.end() && it->m_lsb <= index) {
        return VlForceArrayIndexer<TRhsArray>::elem(rhsArray[it->m_forceId],
                                                    static_cast<std::size_t>(index));
    }
    return origVal;
}

template <typename T>
static inline T VlForceReadArray(const T& origVal, const VlForceVec& forceVec, const T* rhsArray) {
    if (forceVec.empty()) return origVal;

    T result = origVal;
    const int total = static_cast<int>(VlForceArrayIndexer<T>::size);
    for (const auto& entry : forceVec.entries()) {
        const int startIdx = std::max(0, entry.m_lsb);
        const int endIdx = std::min(total - 1, entry.m_msb);
        for (int idx = startIdx; idx <= endIdx; ++idx) {
            const std::size_t uidx = static_cast<std::size_t>(idx);
            VlForceArrayIndexer<T>::elem(result, uidx)
                = VlForceArrayIndexer<T>::elem(rhsArray[entry.m_forceId], uidx);
        }
    }
    return result;
}

#endif  // guard
