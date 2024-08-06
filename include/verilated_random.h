// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
//
// Code available from: https://verilator.org
//
// Copyright 2024 by Wilson Snyder.  This program is free software; you can
// redistribute it and/or modify it under the terms of either the GNU Lesser
// General Public License Version 3 or the Perl Artistic License Version 2.0.
// SPDX-License-Identifier: LGPL-3.0-only OR Artistic-2.0
//
//*************************************************************************
///
/// \file
/// \brief Verilated randomization header
///
/// This file is included automatically by Verilator in some of the C++ files
/// it generates if randomization features are used.
///
/// This file is not part of the Verilated public-facing API.
/// It is only for internal use.
///
/// See the internals documentation docs/internals.rst for details.
///
//*************************************************************************
#ifndef VERILATOR_VERILATED_RANDOM_H_
#define VERILATOR_VERILATED_RANDOM_H_

#include "verilatedos.h"

#include "verilated.h"

#include <functional>
#include <istream>
#include <memory>
#include <utility>

//=============================================================================
// VlRandomExpr and subclasses represent expressions for the constraint solver.

class VlRandomSort;

class VlRandomExpr VL_NOT_FINAL {
public:
    virtual void emit(std::ostream& s) const = 0;
    virtual VlRandomSort sort() const = 0;
    virtual std::unique_ptr<VlRandomExpr> cloneExpr() const = 0;
};

struct VlRandomRandModeIdx VL_NOT_FINAL {
    uint32_t randModeIdx;  // rand_mode index
    bool randModeIdxNone() const { return randModeIdx == std::numeric_limits<uint32_t>::max(); }
};

struct VlRandomVarRef final : public VlRandomRandModeIdx {
    std::string const name;  // Variable name
    void* const datap;  // Reference to variable data
    const size_t width;  // Variable width in bits

    VlRandomVarRef(std::string&& name_, size_t width_, void* datap_, uint32_t randModeIdx_)
        : name{std::move(name_)}
        , datap{datap_}
        , width{width_} {
        randModeIdx = randModeIdx_;
    }
};

struct VlRandomArrRef final : public VlRandomRandModeIdx {
    using GetLengthT = size_t (*const)(void* containerPtr);
    using AccessT = void* (*const)(void* containerPtr, size_t idx);

    std::string const arrName;  // Array variable name
    void* const datap;  // Reference to variable data
    const size_t width;  // Element width
    const GetLengthT getLength;  // Get number of elements
    const AccessT getIdxPtr;  // Get pointer to an element given index
    size_t constrainedIdxCount;  // Number of constrained indices

    VlRandomArrRef(std::string&& arrName_, GetLengthT getLength_, size_t width_, void* datap_,
                   AccessT getIdxPtr_, uint32_t randModeIdx_)
        : arrName{std::move(arrName_)}
        , datap{datap_}
        , width{width_}
        , getLength{getLength_}
        , getIdxPtr{getIdxPtr_} {
        randModeIdx = randModeIdx_;
    }
};

struct VlRandomArrIdxRef final {
    std::string const arrName;  // Array variable name
    int idx;  // Array index
    void* const datap;  // Reference to variable data
    const int width;  // Variable width in bits

    VlRandomArrIdxRef(std::string&& arrName_, int idx_, int width_, void* datap_)
        : arrName{std::move(arrName_)}
        , idx(idx_)
        , datap{datap_}
        , width{width_} {}
};

struct VlRandomInternalVarRef final {
    std::string value;
    size_t width;
};

//=============================================================================
// VlRandomizer is the object holding constraints and variable references.

class VlRandomizer final {
    // MEMBERS
    std::vector<std::string> m_constraints;  // Solver-dependent constraints
    std::map<std::string, VlRandomVarRef> m_vars;  // Solver-dependent variables
    std::map<std::string, VlRandomInternalVarRef>
        m_internalVars;  // Solver-depenedent variables (internal)
    std::map<std::string, VlRandomArrRef> m_arrs;  // Sover-dependent arrays
    const VlQueue<CData>* m_randmode;  // rand_mode state;

    // PRIVATE METHODS
    std::unique_ptr<const VlRandomExpr> randomConstraint(VlRNG& rngr, int bits);
    bool checkSat(std::iostream& file) const;
    void parseSolution(std::iostream& file);
    size_t emitConcatAll(std::ostream& os) const;

public:
    // METHODS
    // Finds the next solution satisfying the constraints
    bool next(VlRNG& rngr);
    template <typename T>
    void writeVar(T& var, int width, const char* name,
                  uint32_t randmodeIdx = std::numeric_limits<uint32_t>::max()) {
        auto it = m_vars.find(name);
        if (it != m_vars.end()) return;
        m_vars.emplace(name, VlRandomVarRef{name, (size_t)width, &var, randmodeIdx});
    }
    template <typename T>
    void writeArr(T& arr, VlRandomArrRef::GetLengthT getLength, size_t width, const char* name,
                  VlRandomArrRef::AccessT idxAccess,
                  uint32_t randmodeIdx = std::numeric_limits<uint32_t>::max()) {
        auto it = m_vars.find(name);
        if (it != m_vars.end()) return;
        m_arrs.emplace(name, VlRandomArrRef{name, getLength, width, &arr, idxAccess, randmodeIdx});
    }
    void hard(std::string&& constraint);
    std::string constrainIndex(const std::string& arrName, std::string&& constraint);
    void clear();
    void set_randmode(const VlQueue<CData>& randmode) { m_randmode = &randmode; }
#ifdef VL_DEBUG
    void dump() const;
#endif
};

#endif  // Guard
