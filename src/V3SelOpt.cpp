// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator:
//
// Code available from: https://verilator.org
//
//*************************************************************************
//
// Copyright 2003-2025 by Wilson Snyder. This program is free software; you
// can redistribute it and/or modify it under the terms of either the GNU
// Lesser General Public License Version 3 or the Perl Artistic License
// Version 2.0.
// SPDX-License-Identifier: LGPL-3.0-only OR Artistic-2.0
//
//*************************************************************************
// V3SelOpt's Transformations:
//
//*************************************************************************

#include "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#include "V3SelOpt.h"

#include "V3Stats.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

class SelOptVisitor final : public VNVisitor {
    VDouble0 m_selsOptimized;
    void visit(AstSel* nodep) override {
        AstConst* const lsbp = VN_CAST(nodep->lsbp(), Const);
        AstConst* const widthp = VN_CAST(nodep->widthp(), Const);

        if (!lsbp || !widthp) return;
        // FIXME bound check for lsbp and widthp to not exceed an int

        VNRelinker relinker;
        nodep->unlinkFrBack(&relinker);
        FileLine* const flp = nodep->fileline();
        relinker.relink(new AstSelNumber{flp, nodep->fromp()->unlinkFrBack(), lsbp->toSInt(),
                                         widthp->toSInt()});
        VL_DO_DANGLING(nodep->deleteTree(), nodep);
        ++m_selsOptimized;
    }
    void visit(AstNode* nodep) override { iterateChildren(nodep); }

public:
    // CONSTRUCTORS
    explicit SelOptVisitor(AstNetlist* nodep) {
        iterate(nodep);
        V3Stats::addStatSum("SelOpt, Sels optimized", m_selsOptimized);
    }
    ~SelOptVisitor() override = default;
};

//######################################################################
// SelOpt class functions

void V3SelOpt::optimize(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ": " << endl);
    { SelOptVisitor{nodep}; }  // Destruct before checking
    V3Global::dumpCheckGlobalTree("sel-opt", 0, dumpTreeEitherLevel() >= 3);
}
