// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Create triggers necessary for scheduling across
//                         virtual interfaces
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
// Each interface type written to via virtual interface, or written to normally but read via
// virtual interface:
//     Create a trigger var for it
// Each AssignW:
//     If it writes to a virtual interface, or to a variable read via virtual interface:
//         Convert to an always
// Each statement:
//     If it writes to a virtual interface, or to a variable read via virtual interface:
//         Set the corresponding trigger to 1
//         If the write is done by an AssignDly, the trigger is also set by AssignDly
//
//*************************************************************************

#include "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#include "V3VirtIface.h"

#include "V3AstNodeExpr.h"
#include "V3UniqueNames.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
//

class VirtIfaceVisitor final : public VNVisitor {
private:
    // NODE STATE
    // AstIface::user1() -> AstVarScope*. Trigger var for this interface
    const VNUser1InUse m_user1InUse;

    // TYPES
    using OnWriteToVirtIface = std::function<void(AstVarRef*, AstIface*)>;

    // STATE
    AstNetlist* const m_netlistp;  // Root node
    AstNodeAssign* m_trigAssignp = nullptr;  // Previous/current trigger assignment
    AstIface* m_trigAssignIfacep = nullptr;  // Interface type whose trigger is assigned
                                             // by m_trigAssignp
    V3UniqueNames m_vifTriggerNames{"__VvifTrigger"};  // Unique names for virt iface
                                                       // triggers

    // METHODS
    static void foreachWrittenVirtIface(AstNode* const nodep, const OnWriteToVirtIface& onWrite) {
        nodep->foreach([&](AstVarRef* const refp) {
            if (refp->access().isReadOnly()) return;
            if (AstIfaceRefDType* dtypep = VN_CAST(refp->varp()->dtypep(), IfaceRefDType)) {
                if (dtypep->isVirtual() && VN_IS(refp->firstAbovep(), MemberSel)) {
                    onWrite(refp, dtypep->ifacep());
                }
            } else if (AstIface* ifacep = refp->varp()->sensIfacep()) {
                onWrite(refp, ifacep);
            }
        });
    }
    static void unsupportedWriteToVirtIface(AstNode* nodep) {
        if (!nodep) return;
        foreachWrittenVirtIface(nodep, [](AstVarRef* const selp, AstIface*) {
            selp->v3warn(E_UNSUPPORTED,
                         "Unsupported: write to virtual interface in this location");
        });
    }
    AstVarRef* createVirtIfaceTriggerRefp(FileLine* const flp, AstIface* ifacep) {
        if (!ifacep->user1()) {
            AstScope* const scopeTopp = m_netlistp->topScopep()->scopep();
            auto* vscp = scopeTopp->createTemp(m_vifTriggerNames.get(ifacep), 1);
            ifacep->user1p(vscp);
            m_netlistp->virtIfaceTriggerps().push_back(std::make_pair(ifacep, vscp));
        }
        return new AstVarRef{flp, VN_AS(ifacep->user1p(), VarScope), VAccess::WRITE};
    }

    // VISITORS
    void visit(AstNodeProcedure* nodep) override {
        VL_RESTORER(m_trigAssignp);
        VL_RESTORER(m_trigAssignIfacep);
        m_trigAssignp = nullptr;
        m_trigAssignIfacep = nullptr;
        iterateChildren(nodep);
    }
    void visit(AstCFunc* nodep) override {
        VL_RESTORER(m_trigAssignp);
        VL_RESTORER(m_trigAssignIfacep);
        m_trigAssignp = nullptr;
        m_trigAssignIfacep = nullptr;
        iterateChildren(nodep);
    }
    void visit(AstAssignW* nodep) override {
        if (nodep->exists([](AstVarRef* const refp) {
                AstIfaceRefDType* dtypep = VN_CAST(refp->varp()->dtypep(), IfaceRefDType);
                return refp->access().isWriteOrRW()
                       && ((dtypep && dtypep->isVirtual() && VN_IS(refp->firstAbovep(), MemberSel))
                           || refp->varp()->sensIfacep());
            })) {
            nodep->convertToAlways();
        }
    }
    void visit(AstNodeIf* nodep) override {
        unsupportedWriteToVirtIface(nodep->condp());
        m_trigAssignp = nullptr;
        iterateAndNextNull(nodep->thensp());
        m_trigAssignp = nullptr;
        iterateAndNextNull(nodep->elsesp());
    }
    void visit(AstWhile* nodep) override {
        unsupportedWriteToVirtIface(nodep->precondsp());
        unsupportedWriteToVirtIface(nodep->condp());
        unsupportedWriteToVirtIface(nodep->incsp());
        m_trigAssignp = nullptr;
        iterateAndNextNull(nodep->stmtsp());
    }
    void visit(AstNodeStmt* nodep) override {
        if (nodep->user1SetOnce()) return;  // Process once
        if (nodep->isTimingControl()) {
            m_trigAssignp = nullptr;  // Could be after a delay - need new trigger assignment
        }
        FileLine* const flp = nodep->fileline();
        foreachWrittenVirtIface(nodep, [&](AstVarRef*, AstIface* ifacep) {
            if (ifacep != m_trigAssignIfacep) {
                m_trigAssignIfacep = ifacep;
                m_trigAssignp = nullptr;
            }
            if (VN_IS(nodep, AssignDly)) {
                if (!VN_IS(m_trigAssignp, AssignDly)) {
                    m_trigAssignp = new AstAssignDly{flp, createVirtIfaceTriggerRefp(flp, ifacep),
                                                     new AstConst{flp, AstConst::BitTrue{}}};
                    AstNode* addp = nodep;
                    if (VN_IS(nodep->backp(), Fork)) addp = nodep->backp();
                    addp->addHereThisAsNext(m_trigAssignp);
                }
            } else if (!m_trigAssignp || VN_IS(m_trigAssignp, AssignDly)) {
                m_trigAssignp = new AstAssign{flp, createVirtIfaceTriggerRefp(flp, ifacep),
                                              new AstConst{flp, AstConst::BitTrue{}}};
                nodep->addNextHere(m_trigAssignp);
            }
        });
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

void V3VirtIface::makeTriggers(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ": " << endl);
    { VirtIfaceVisitor{nodep}; }
    V3Global::dumpCheckGlobalTree("vif", 0, dumpTreeLevel() >= 3);
}
