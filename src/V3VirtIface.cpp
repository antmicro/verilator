// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Create separate tasks for forked processes that
//              can outlive their parents
//
// Code available from: https://verilator.org
//
//*************************************************************************
//
// Copyright 2003-2023 by Wilson Snyder. This program is free software; you
// can redistribute it and/or modify it under the terms of either the GNU
// Lesser General Public License Version 3 or the Perl Artistic License
// Version 2.0.
// SPDX-License-Identifier: LGPL-3.0-only OR Artistic-2.0
//
//*************************************************************************
// V3VirtIface's Transformations:
//
//*************************************************************************

#include "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#include "V3VirtIface.h"

#include "V3AstNodeExpr.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
//

class VirtIfaceVisitor final : public VNVisitor {
private:
    // NODE STATE
    // AstNode::user1()         -> bool, 1 = Node was created as a call to an asynchronous task
    // AstVarRef::user2()       -> bool, 1 = Node is a class handle reference. The handle gets
    //                                       modified in the context of this reference.
    const VNUser1InUse m_inuser1;
    const VNUser2InUse m_inuser2;

    // STATE
    AstNetlist* const m_netlistp;  // Root node
    AstNodeAssign* m_trigAssignp = nullptr;  // Previous/current trigger assignment

    // METHODS
    static bool writesToVirtIface(AstNodeStmt* nodep) {
        return nodep->exists([](const AstVarRef* varrefp) {
            return varrefp->access().isWriteOrRW() && varrefp->varp()->isVirtIface();
        });
    }
    AstVarRef* createVirtIfaceTriggerRefp(FileLine* const flp) const {
        if (!m_netlistp->virtIfaceTriggerp()) {
            AstScope* const scopeTopp = m_netlistp->topScopep()->scopep();
            m_netlistp->virtIfaceTriggerp(scopeTopp->createTemp("__VvirtIfaceTrigger", 1));
        }
        return new AstVarRef{flp, m_netlistp->virtIfaceTriggerp(), VAccess::WRITE};
    }

    // VISITORS
    void visit(AstNodeProcedure* nodep) override {
        m_trigAssignp = nullptr;
        iterateChildren(nodep);
    }
    void visit(AstCFunc* nodep) override {
        m_trigAssignp = nullptr;
        iterateChildren(nodep);
    }
    void visit(AstAssignW* nodep) override {
        if (writesToVirtIface(nodep)) { nodep->convertToAlways(); }
    }
    void visit(AstNodeIf* nodep) override {
        // TODO: handle ->condp()
        m_trigAssignp = nullptr;
        iterateAndNextNull(nodep->thensp());
        m_trigAssignp = nullptr;
        iterateAndNextNull(nodep->elsesp());
    }
    void visit(AstWhile* nodep) override {
        // TODO: handle ->condp() etc.
        m_trigAssignp = nullptr;
        iterateAndNextNull(nodep->stmtsp());
    }
    void visit(AstNodeStmt* nodep) override {
        if (!VN_IS(nodep, NodeAssign)  //
            || !VN_IS(nodep, StmtExpr)  //
            || nodep->isTimingControl()) {
            m_trigAssignp = nullptr;
        }
        if (!writesToVirtIface(nodep)) return;
        FileLine* const flp = nodep->fileline();
        if (m_trigAssignp) {
            m_trigAssignp->unlinkFrBack();
        } else if (VN_IS(nodep, AssignDly)) {
            m_trigAssignp = new AstAssignDly{flp, createVirtIfaceTriggerRefp(flp),
                                             new AstConst{flp, AstConst::BitTrue{}}};
        } else {
            m_trigAssignp = new AstAssign{flp, createVirtIfaceTriggerRefp(flp),
                                          new AstConst{flp, AstConst::BitTrue{}}};
        }
        nodep->addNextHere(m_trigAssignp);
    }
    void visit(AstNodeExpr*) override {}  // Accelerate
    void visit(AstNode* nodep) override { iterateChildren(nodep); }

public:
    // CONSTRUCTORS
    explicit VirtIfaceVisitor(AstNetlist* nodep)
        : m_netlistp{nodep} {
        iterate(nodep);
    }
    ~VirtIfaceVisitor() override = default;
};

//######################################################################
// VirtIface class functions

void V3VirtIface::makeTriggerAssignments(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ": " << endl);
    { VirtIfaceVisitor{nodep}; }
    V3Global::dumpCheckGlobalTree("vif", 0, dumpTreeLevel() >= 3);
}
