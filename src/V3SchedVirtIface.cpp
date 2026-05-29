// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Create triggers necessary for scheduling across
//                         virtual interfaces
//
// Code available from: https://verilator.org
//
//*************************************************************************
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of either the GNU Lesser General Public License Version 3
// or the Perl Artistic License Version 2.0.
// SPDX-FileCopyrightText: 2003-2026 Wilson Snyder
// SPDX-License-Identifier: LGPL-3.0-only OR Artistic-2.0
//
//*************************************************************************
// V3SchedVirtIface's Transformations:
//
// Identify interface members written through virtual interface variables (VIF writes).
// For each such member, collect all VarScopes across all instantiations of that
// interface type. Each VarScope gets its own value-change trigger in the scheduler.
//
// Also disables lifetime optimization for variables in AlwaysPost blocks that
// write through virtual interfaces.
//
//*************************************************************************

#include "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#include "V3AstNodeExpr.h"
#include "V3Sched.h"

VL_DEFINE_DEBUG_FUNCTIONS;

namespace V3Sched {

namespace {

class VirtIfaceVisitor final : public VNVisitor {
private:
    // STATE
    // Set of (iface, member) pairs written through VIF -- defines which members need triggers
    std::set<std::pair<const AstIface*, const std::string>> m_vifWrittenMembers;
    // All candidate VarScopes of interface members (keyed by interface type + member name)
    std::map<std::pair<const AstIface*, const std::string>, std::vector<AstVarScope*>>
        m_candidateVscps;
    VirtIfaceTriggers m_triggers;
    using IfaceCallable = std::pair<AstIface*, AstNode*>;
    std::vector<IfaceCallable> m_reachableIfaceCallables;

    // METHODS
    void addCandidateVscp(const AstIface* const ifacep, AstVarScope* const vscp) {
        std::vector<AstVarScope*>& vscps = m_candidateVscps[{ifacep, vscp->varp()->name()}];
        if (std::find(vscps.begin(), vscps.end(), vscp) == vscps.end()) vscps.push_back(vscp);
    }

    static bool isConcreteIfaceMemberRef(const AstVarRef* const refp) {
        // Concrete interface member reference is a VarRef that accesses an instance of an
        // interface (not a virtual interface) and is a write access
        UASSERT_OBJ(refp->varScopep(), refp, "No var scope");
        return refp->access().isWriteOrRW() && !refp->varp()->isFuncLocal()
               && !refp->varp()->isTemp() && !refp->varp()->isEvent()
               && VN_IS(refp->varScopep()->scopep()->modp(), Iface);
    }

    void collectConcreteIfaceWrite(AstIface* const ifacep, AstVarRef* const refp) {
        if (!isConcreteIfaceMemberRef(refp)) return;
        refp->varp()->sensIfacep(ifacep);
        m_vifWrittenMembers.emplace(ifacep, refp->varp()->name());
        addCandidateVscp(ifacep, refp->varScopep());
    }

    // VISITORS
    void visit(AstMemberSel* nodep) override {
        // Detect writes through VIF: the MemberSel's fromp resolves to a virtual interface type.
        // Handles both direct VIF access (vif.member) and class chain (obj.vif.member).
        if (nodep->access().isWriteOrRW()) {
            if (const AstIfaceRefDType* const dtypep = nodep->fromp()->dtypeIfVirtualIfacep()) {
                m_vifWrittenMembers.emplace(dtypep->ifacep(), nodep->varp()->name());
            }
        }
        iterateChildren(nodep);
    }
    void visit(AstCMethodCall* nodep) override {
        const AstIfaceRefDType* const dtypep = nodep->fromp()->dtypeIfVirtualIfacep();
        if (!dtypep) return;
        m_reachableIfaceCallables.emplace_back(dtypep->ifacep(), nodep->funcp());
        iterateChildren(nodep);
    }
    void visit(AstVarScope* nodep) override {
        // Collect candidate VarScopes. sensIfacep() is set on interface members
        // accessed via any MemberSel (virtual or non-virtual).
        if (const AstIface* const ifacep = nodep->varp()->sensIfacep()) {
            if (!nodep->varp()->isTemp()) { addCandidateVscp(ifacep, nodep); }
        }
    }
    void visit(AstNodeProcedure* nodep) override {
        // Disable lifetime optimization for variables in AlwaysPost blocks
        // that write to virtual interface members, as VIF reads may observe them
        if (VN_IS(nodep, AlwaysPost)) {
            nodep->foreach([](AstVarRef* refp) { refp->varScopep()->optimizeLifePost(false); });
        }
        iterateChildren(nodep);
    }
    void visit(AstNode* nodep) override { iterateChildren(nodep); }

    // Build final trigger list by intersecting VIF writes with candidate VarScopes
    void buildTriggers() {
        for (const auto& written : m_vifWrittenMembers) {
            const auto it = m_candidateVscps.find(written);
            if (it == m_candidateVscps.end()) continue;
            for (AstVarScope* const vscp : it->second) {
                const AstIface* const ifacep = written.first;
                m_triggers.addTrigger(ifacep, vscp->varp(), vscp);
            }
        }
    }

public:
    // CONSTRUCTORS
    explicit VirtIfaceVisitor(AstNetlist* nodep) {
        iterate(nodep);
        for (const IfaceCallable& pair : m_reachableIfaceCallables) {
            pair.second->foreach([this, pair](AstNodeExpr* const nodep) {
                if (AstVarRef* const refp = VN_CAST(nodep, VarRef)) {
                    collectConcreteIfaceWrite(pair.first, refp);
                } else if (AstNodeCCall* const callp = VN_CAST(nodep, NodeCCall)) {
                    m_reachableIfaceCallables.emplace_back(pair.first, callp->funcp());
                }
            });
        }
        buildTriggers();
    }
    ~VirtIfaceVisitor() override = default;

    // METHODS
    VirtIfaceTriggers take_triggers() { return std::move(m_triggers); }
};

}  //namespace

VirtIfaceTriggers makeVirtIfaceTriggers(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    VirtIfaceTriggers triggers{};
    if (v3Global.hasVirtIfaces()) {
        triggers = VirtIfaceVisitor{nodep}.take_triggers();
        // Dump after destructor so VNDeleter runs
        V3Global::dumpCheckGlobalTree("sched_vif", 0, dumpTreeEitherLevel() >= 6);
    }
    return triggers;
}

}  //namespace V3Sched
