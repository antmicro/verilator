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

//=============================================================================
// VlRandomExpr and subclasses represent expressions for the constraint solver.

class VlRandomSort;

class VlRandomExpr VL_NOT_FINAL {
public:
    virtual void emit(std::ostream& s) const = 0;
    virtual VlRandomSort sort() const = 0;
};

class VlRandomSort final : public VlRandomExpr {
public:
    enum class Constructor { VL_RAND_BV, VL_RAND_ABV, VL_RAND_BOOL, VL_RAND_SORT };

private:
    Constructor m_ty;

    int m_elem_width;
    int m_len;

    VlRandomSort(Constructor ty, int elem_width, int len)
        : m_ty(ty)
        , m_elem_width(elem_width)
        , m_len(len) {}

public:
    static VlRandomSort BV(int width) { return VlRandomSort(Constructor::VL_RAND_BV, width, 0); }
    static VlRandomSort ABV(int width, int len) {
        return VlRandomSort(Constructor::VL_RAND_ABV, width, len);
    }
    static VlRandomSort Bool() { return VlRandomSort(Constructor::VL_RAND_BOOL, 1, 0); }
    static VlRandomSort Sort() { return VlRandomSort(Constructor::VL_RAND_SORT, 0, 0); }

    int elemWidth() const { return m_elem_width; }
    int len() const { return m_len; }
    int idxWidth() const {
#ifndef _WIN32
        if (m_len == 0) return 1;
        return __builtin_clz(m_len);
#else
        int i = m_len;
        i |= (i >> 1);
        i |= (i >> 2);
        i |= (i >> 4);
        i |= (i >> 8);
        i |= (i >> 16);
        i -= ((i >> 1) & 0x55555555);
        i = (((i >> 2) & 0x33333333) + (i & 0x33333333));
        i = (((i >> 4) + i) & 0x0f0f0f0f);
        i += (i >> 8);
        i += (i >> 16);
        return i & 0x0000003f;
#endif
    }
    Constructor constructor() const { return m_ty; }

    void emit(std::ostream& s) const override;
    VlRandomSort sort() const override { return Sort(); }
};

class VlRandomRef VL_NOT_FINAL : public VlRandomExpr {
public:
    virtual bool set(std::string&&) const = 0;
};

class VlRandomVarRef final : public VlRandomRef {
    const char* const m_name;  // Variable name
    void* const m_datap;  // Reference to variable data
    const VlRandomSort m_sort;  // Variable width in bits
    const uint32_t m_randModeIdx;  // rand_mode index

public:
    VlRandomVarRef(const char* name, VlRandomSort sort, void* datap, uint32_t randModeIdx)
        : m_name{name}
        , m_datap{datap}
        , m_sort{sort}
        , m_randModeIdx{randModeIdx} {}
    const char* name() const { return m_name; }
    VlRandomSort sort() const override { return m_sort; }
    void* datap() const { return m_datap; }
    std::uint32_t randModeIdx() const { return m_randModeIdx; }
    bool randModeIdxNone() const { return randModeIdx() == std::numeric_limits<unsigned>::max(); }
    bool set(std::string&&) const override;
    void emit(std::ostream& s) const override;
};

class VlRandomBVConst final : public VlRandomExpr {
    const QData m_val;  // Constant value
    const int m_width;  // Constant width in bits

public:
    VlRandomBVConst(QData val, int width)
        : m_val{val}
        , m_width{width} {
        assert(width <= sizeof(m_val) * 8);
    }
    void emit(std::ostream& s) const override;
    VlRandomSort sort() const override { return VlRandomSort::BV(m_width); }
};

class VlRandomExtract final : public VlRandomExpr {
    const std::shared_ptr<const VlRandomExpr> m_expr;  // Sub-expression
    const unsigned m_idx;  // Extracted index

public:
    VlRandomExtract(std::shared_ptr<const VlRandomExpr> expr, unsigned idx)
        : m_expr{expr}
        , m_idx{idx} {}
    void emit(std::ostream& s) const override;
    VlRandomSort sort() const override { return VlRandomSort::BV(1); }
};

class VlRandomBinOp VL_NOT_FINAL : public VlRandomExpr {
protected:
    const std::shared_ptr<const VlRandomExpr> m_lhs, m_rhs;  // Sub-expressions
    const char* m_op;
    VlRandomBinOp(const char* op, std::shared_ptr<const VlRandomExpr> lhs,
                  std::shared_ptr<const VlRandomExpr> rhs)
        : m_op(op)
        , m_lhs{lhs}
        , m_rhs{rhs} {}

public:
    void emit(std::ostream& s) const override;
};

class VlRandomBVXor final : public VlRandomBinOp {
public:
    VlRandomBVXor(std::shared_ptr<const VlRandomExpr> lhs, std::shared_ptr<const VlRandomExpr> rhs)
        : VlRandomBinOp("bvxor", std::move(lhs), std::move(rhs)) {}
    VlRandomSort sort() const override { return m_lhs->sort(); }
};

class VlRandomConcat final : public VlRandomBinOp {
public:
    VlRandomConcat(std::shared_ptr<const VlRandomExpr> lhs,
                   std::shared_ptr<const VlRandomExpr> rhs)
        : VlRandomBinOp("bvxor", std::move(lhs), std::move(rhs)) {}
    VlRandomSort sort() const override {
        return VlRandomSort::BV(m_lhs->sort().elemWidth() + m_rhs->sort().elemWidth());
    }
};

class VlRandomEq final : public VlRandomBinOp {
public:
    VlRandomEq(std::shared_ptr<const VlRandomExpr> lhs, std::shared_ptr<const VlRandomExpr> rhs)
        : VlRandomBinOp("=", std::move(lhs), std::move(rhs)) {}
    VlRandomSort sort() const override { return VlRandomSort::Bool(); }
};

class VlRandomSelect final : public VlRandomBinOp {
public:
    VlRandomSelect(std::shared_ptr<const VlRandomExpr> lhs,
                   std::shared_ptr<const VlRandomExpr> rhs)
        : VlRandomBinOp("select", std::move(lhs), std::move(rhs)) {}
    VlRandomSort sort() const override { return VlRandomSort::BV(m_lhs->sort().elemWidth()); }
};

class VlRandomSelectRef final : public VlRandomRef {
    const std::shared_ptr<const VlRandomRef> m_lhs;
    int m_idx;

public:
    VlRandomSelectRef(std::shared_ptr<const VlRandomRef> lhs, int idx)
        : m_lhs{lhs}
        , m_idx{idx} {}
    void emit(std::ostream& s) const override;
    VlRandomSort sort() const override { return VlRandomSort::BV(m_lhs->sort().elemWidth()); }
    bool set(std::string&&) const override;
};

//=============================================================================
// VlRandomizer is the object holding constraints and variable references.

class VlRandomizer final {
    // MEMBERS
    std::vector<std::string> m_constraints;  // Solver-dependent constraints
    std::map<std::string, std::shared_ptr<const VlRandomRef>>
        m_vars;  // Solver-dependent variables
    const VlQueue<CData>* m_randmode;  // rand_mode state;

    // PRIVATE METHODS
    std::shared_ptr<const VlRandomExpr> randomConstraint(VlRNG& rngr, int bits);
    bool parseSolution(std::iostream& file);

public:
    // METHODS
    // Finds the next solution satisfying the constraints
    bool next(VlRNG& rngr);
    template <typename T>
    void write_var(T& var, int width, const char* name,
                   std::uint32_t randmodeIdx = std::numeric_limits<std::uint32_t>::max()) {
        auto it = m_vars.find(name);
        if (it != m_vars.end()) return;
        m_vars[name] = std::make_shared<const VlRandomVarRef>(name, VlRandomSort::BV(width), &var,
                                                              randmodeIdx);
    }
    void hard(std::string&& constraint);
    void clear();
    void set_randmode(const VlQueue<CData>& randmode) { m_randmode = &randmode; }
#ifdef VL_DEBUG
    void dump() const;
#endif
};

#endif  // Guard
