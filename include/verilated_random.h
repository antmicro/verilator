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
#include <istream>
#include <memory>

//=============================================================================
// VlRandomExpr and subclasses represent expressions for the constraint solver.

class VlRandomSort;

class VlRandomExpr VL_NOT_FINAL {
public:
    virtual void emit(std::ostream& s) const = 0;
    virtual VlRandomSort sort() const = 0;
    virtual std::unique_ptr<VlRandomExpr> cloneExpr() const = 0;
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
    std::unique_ptr<VlRandomExpr> cloneExpr() const override {
        return std::make_unique<VlRandomSort>(*this);
    }
};

class VlRandomRef VL_NOT_FINAL : public VlRandomExpr {
public:
    virtual bool set(std::string&&) const = 0;
    std::unique_ptr<VlRandomRef> cloneRef() const {
        return std::unique_ptr<VlRandomRef>(static_cast<VlRandomRef*>(cloneExpr().release()));
    }
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
    std::unique_ptr<VlRandomExpr> cloneExpr() const override {
        return std::make_unique<VlRandomVarRef>(*this);
    }
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
    std::unique_ptr<VlRandomExpr> cloneExpr() const override {
        return std::make_unique<VlRandomBVConst>(*this);
    }
};

class VlRandomExtract final : public VlRandomExpr {
    const std::unique_ptr<const VlRandomExpr> m_expr;  // Sub-expression
    const unsigned m_idx;  // Extracted index

public:
    VlRandomExtract(std::unique_ptr<const VlRandomExpr>&& expr, unsigned idx)
        : m_expr{std::move(expr)}
        , m_idx{idx} {}
    void emit(std::ostream& s) const override;
    VlRandomSort sort() const override { return VlRandomSort::BV(1); }
    std::unique_ptr<VlRandomExpr> cloneExpr() const override {
        return std::make_unique<VlRandomExtract>(m_expr->cloneExpr(), m_idx);
    }
};

class VlRandomBinOp VL_NOT_FINAL : public VlRandomExpr {
protected:
    const std::unique_ptr<const VlRandomExpr> m_lhs, m_rhs;  // Sub-expressions
    const char* m_op;
    VlRandomBinOp(const char* op, std::unique_ptr<const VlRandomExpr> lhs,
                  std::unique_ptr<const VlRandomExpr> rhs)
        : m_op(op)
        , m_lhs{std::move(lhs)}
        , m_rhs{std::move(rhs)} {}

public:
    void emit(std::ostream& s) const override;
};

class VlRandomBVXor final : public VlRandomBinOp {
public:
    VlRandomBVXor(std::unique_ptr<const VlRandomExpr> lhs, std::unique_ptr<const VlRandomExpr> rhs)
        : VlRandomBinOp("bvxor", std::move(lhs), std::move(rhs)) {}
    VlRandomSort sort() const override { return m_lhs->sort(); }
    std::unique_ptr<VlRandomExpr> cloneExpr() const override {
        return std::make_unique<VlRandomBVXor>(m_lhs->cloneExpr(), m_rhs->cloneExpr());
    }
};

class VlRandomBVXorMany final : public VlRandomExpr {
    std::vector<std::unique_ptr<const VlRandomExpr>> m_exprs;
public:
    VlRandomBVXorMany(std::vector<std::unique_ptr<const VlRandomExpr>>&& exprs)
        : m_exprs{std::move(exprs)} {}
    void emit(std::ostream& s) const override;
    VlRandomSort sort() const override { return m_exprs[0]->sort(); }
    std::unique_ptr<VlRandomExpr> cloneExpr() const override {
        std::vector<std::unique_ptr<const VlRandomExpr>> new_exprs;
        for (auto& expr : m_exprs) { new_exprs.emplace_back(expr->cloneExpr()); }
        return std::make_unique<VlRandomBVXorMany>(std::move(new_exprs));
    }
};

class VlRandomConcat final : public VlRandomBinOp {
public:
    VlRandomConcat(std::unique_ptr<const VlRandomExpr> lhs,
                   std::unique_ptr<const VlRandomExpr> rhs)
        : VlRandomBinOp("concat", std::move(lhs), std::move(rhs)) {}
    VlRandomSort sort() const override {
        return VlRandomSort::BV(m_lhs->sort().elemWidth() + m_rhs->sort().elemWidth());
    }
    std::unique_ptr<VlRandomExpr> cloneExpr() const override {
        return std::make_unique<VlRandomConcat>(m_lhs->cloneExpr(), m_rhs->cloneExpr()));
    }
};

class VlRandomEq final : public VlRandomBinOp {
public:
    VlRandomEq(std::unique_ptr<const VlRandomExpr> lhs, std::unique_ptr<const VlRandomExpr> rhs)
        : VlRandomBinOp("=", std::move(lhs), std::move(rhs)) {}
    VlRandomSort sort() const override { return VlRandomSort::Bool(); }
    std::unique_ptr<VlRandomExpr> cloneExpr() const override {
        return std::make_unique<VlRandomEq>(m_lhs->cloneExpr(), m_rhs->cloneExpr());
    }
};

class VlRandomSelect final : public VlRandomBinOp {
public:
    VlRandomSelect(std::unique_ptr<const VlRandomExpr> lhs,
                   std::unique_ptr<const VlRandomExpr> rhs)
        : VlRandomBinOp("select", std::move(lhs), std::move(rhs)) {}
    VlRandomSort sort() const override { return VlRandomSort::BV(m_lhs->sort().elemWidth()); }
    std::unique_ptr<VlRandomExpr> cloneExpr() const override {
        return std::make_unique<VlRandomSelect>(m_lhs->cloneExpr(), m_rhs->cloneExpr());
    }
};

class VlRandomSelectRef final : public VlRandomRef {
    const std::unique_ptr<const VlRandomRef> m_lhs;
    int m_idx;

public:
    VlRandomSelectRef(std::unique_ptr<const VlRandomRef>&& lhs, int idx)
        : m_lhs{std::move(lhs)}
        , m_idx{idx} {}
    void emit(std::ostream& s) const override;
    VlRandomSort sort() const override { return VlRandomSort::BV(m_lhs->sort().elemWidth()); }
    bool set(std::string&&) const override;
    std::unique_ptr<VlRandomExpr> cloneExpr() const override {
        return std::make_unique<VlRandomSelectRef>(m_lhs->cloneRef(), m_idx);
    }
};

class VlRandomConstraint final : public VlRandomExpr {
    std::unique_ptr<VlRandomExpr> m_mult, m_mod, m_hash;
    std::vector<std::unique_ptr<const VlRandomExpr>> m_bv_exprs;
public:
    VlRandomConstraint(std::unique_ptr<VlRandomExpr>&& mult, std::unique_ptr<VlRandomExpr>&& mod,
                       std::unique_ptr<VlRandomExpr>&& hash,
                       std::vector<std::unique_ptr<const VlRandomExpr>>&& bv_exprs)
        : m_mult{std::move(mult)}
        , m_mod{std::move(mod)}
        , m_hash{std::move(hash)}
        , m_bv_exprs{std::move(bv_exprs)} {}
    void emit(std::ostream& s) const override;
    VlRandomSort sort() const override { return VlRandomSort::Bool(); }
    std::unique_ptr<VlRandomExpr> cloneExpr() const override {
        std::vector<std::unique_ptr<const VlRandomExpr>> new_exprs;
        for (auto& expr : m_bv_exprs) { new_exprs.emplace_back(expr->cloneExpr()); }
        return std::make_unique<VlRandomConstraint>(m_mult->cloneExpr(), m_mod->cloneExpr(),
                                                    m_hash->cloneExpr(), std::move(new_exprs));
    }
};

//=============================================================================
// VlRandomizer is the object holding constraints and variable references.

class VlRandomizer final {
    // MEMBERS
    std::vector<std::string> m_constraints;  // Solver-dependent constraints
    std::map<std::string, std::unique_ptr<const VlRandomRef>>
        m_vars;  // Solver-dependent variables
    const VlQueue<CData>* m_randmode;  // rand_mode state;

    // PRIVATE METHODS
    std::unique_ptr<const VlRandomExpr> randomConstraint(VlRNG& rngr, int bits);
    bool checkSat(std::iostream& file) const;
    void parseSolution(std::iostream& file);

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
