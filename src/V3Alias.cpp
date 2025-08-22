// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Resolve aliases
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

#include "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#include "V3Alias.h"

#include "V3AstUserAllocator.h"

#include <vector>

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// AliasVisitor

class AliasVisitor final : public VNVisitor {
    // NODE STATE

    // STATE - across all visitors

    // METHODS

    // VISITORS
    void visit(AstNode* nodep) override { iterateChildren(nodep); }

public:
    // CONSTRUCTORS
    explicit AliasVisitor(AstNetlist* nodep) { iterate(nodep); }
};

//######################################################################
// Alias class functions

void V3Alias::alias(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { AliasVisitor{nodep}; }  // Destruct before checking
    V3Global::dumpCheckGlobalTree("alias", 0, dumpTreeEitherLevel() >= 6);
}
