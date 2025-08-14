// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Covert forceable signals, process force/release
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
//  V3Force's Transformations:
//
//  For each forceable var/net with name "<name>":
//      For each `force <name> = <RHS>` (AstAssignForce):
//          - add an extra signal: <name>__VforceEn[ID] and set it on the force range to ones
//          - add an initial assignment: initial <name>__VforceEn[ID] = 0;
//          - store the <RHS[ID]> in a scope
//      Replace all READ references to <name> with the following expression (force read
//      expression):
//          <name>__VforceEn0 & <RHS0> | ... | <name>__VforceEn[LastID] & <LastRHS>
//          | ~(<name>__VforceEn0 | ... | <name>__VforceEn[LastID]) & <name>
//          (or the same with if-else for non-ranged datatypes)
//
//  Replace each AstAssignForce with:
//      - <lhs>__VforceEn[ID] = all ones (on the given range)
//      - for every [ID2] other than[ID], do: <lhs>__VforceEn[ID2] &= ~(<lhs>__VforceEn[ID])
//          (or just single-bit 1 for non-ranged datatypes)
//
//  Replace each AstRelease with:
//      - <lhs> = (force read expression of <lhs>) // iff lhs is not a net
//      - For each [ID]: <lhs>__VforceEn[ID] &= all zeros (on the given range)
//*************************************************************************

#include "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#include "V3Force.h"

#include "V3AstUserAllocator.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Convert force/release statements and signals marked 'forceable'

class ForceState final {
public:
    // TYPES
    struct ForceInfo final {
        AstNodeExpr* m_rhsExprp;  // RHS expression for this force assignment
        AstVarScope* m_enVscp;  // Force enabled signal scope for this ID (__VforceEnID)

        ForceInfo(AstNodeExpr* rhsExprp, AstVarScope* enVscp)
            : m_rhsExprp{rhsExprp}
            , m_enVscp{enVscp} {}
    };

    void addExternalForce(AstVarScope* scopep, AstVar* varp) {
        AstNodeDType* dtypep
            = ForceState::isRangedDType(varp) ? varp->dtypep() : varp->findBitDType();
        AstVar* const enVarp
            = new AstVar{varp->fileline(), VVarType::VAR, varp->name() + "__VforceEn", dtypep};
        enVarp->sigUserRWPublic(true);
        AstVar* const valVarp
            = new AstVar{varp->fileline(), VVarType::VAR, varp->name() + "__VforceVal", dtypep};
        valVarp->sigUserRWPublic(true);
        varp->addNextHere(enVarp);
        varp->addNextHere(valVarp);
        AstVarScope* enVscp = new AstVarScope{varp->fileline(), scopep->scopep(), enVarp};
        AstVarScope* valVscp = new AstVarScope{varp->fileline(), scopep->scopep(), valVarp};
        scopep->scopep()->addVarsp(enVscp);
        scopep->scopep()->addVarsp(valVscp);
        AstSenItem* const itemsp
            = new AstSenItem{varp->fileline(), VEdgeType::ET_CHANGED,
                             new AstVarRef{varp->fileline(), enVscp, VAccess::READ}};
        AstActive* const activep = new AstActive{varp->fileline(), "force-update",
                                                 new AstSenTree{varp->fileline(), itemsp}};
        activep->senTreeStorep(activep->sentreep());
        AstVarRef* refp = new AstVarRef{varp->fileline(), scopep, VAccess::READ};
        ForceState::markNonReplaceable(refp);
        AstNodeExpr* forceExprp = nullptr;
        AstVarRef* enRefp = new AstVarRef{varp->fileline(), enVscp, VAccess::READ};
        AstVarRef* valRefp = new AstVarRef{varp->fileline(), valVscp, VAccess::READ};
        if (isRangedDType(varp)) {
            forceExprp = new AstOr{
                varp->fileline(), new AstAnd{varp->fileline(), enRefp, valRefp},
                new AstAnd{varp->fileline(),
                           new AstNot{varp->fileline(), enRefp->cloneTreePure(false)}, refp}};
        } else {
            forceExprp = new AstCond{varp->fileline(), enRefp, valRefp, refp};
        }
        activep->addStmtsp(new AstAlways{
            varp->fileline(), VAlwaysKwd::ALWAYS, nullptr,
            new AstAssignForce{varp->fileline(),
                               new AstVarRef{varp->fileline(), scopep, VAccess::WRITE},
                               forceExprp}});
        scopep->scopep()->addBlocksp(activep);
    }

    // STATIC METHODS
    // Replace each AstNodeVarRef in the given 'nodep' that writes a variable by transforming the
    // referenced AstVarScope with the given function.
    static void transformVarScopes(AstNode* nodep, AstVarScope* vscp) {
        UASSERT_OBJ(nodep->backp(), nodep, "Must have backp, otherwise will be lost if replaced");
        nodep->foreach([vscp](AstNodeVarRef* refp) {
            refp->replaceWith(new AstVarRef{refp->fileline(), vscp, refp->access()});
            VL_DO_DANGLING(refp->deleteTree(), refp);
        });
    }

    static AstConst* makeZeroConst(AstNode* nodep, int width) {
        V3Number zero{nodep, width};
        zero.setAllBits0();
        return new AstConst{nodep->fileline(), zero};
    }

    // METHODS
    static AstNodeExpr* createForceReadExpression(
        const std::unordered_map<AstAssignForce*, ForceState::ForceInfo>& forces,
        AstVarRef* originalRefp, AstNodeExpr* lhsp) {
        UASSERT(!forces.empty(), "createForceReadExpression with no forces!");

        // Build the force read expression:
        // __VforceEn0 & RHS0 | __VforceEn1 & RHS1 | ... | ~(__VforceEn0 | __VforceEn1 | ...) &
        // original

        // Build the force terms: __VforceEnID & RHSID
        AstNodeExpr* forceExprp = nullptr;
        AstNot* notEnablesp = nullptr;
        for (const auto& force : forces) {
            AstVarScope* const enVscp = force.second.m_enVscp;
            AstNodeExpr* const lhsForceEnp = lhsp->cloneTreePure(false);
            AstNodeExpr* rhsClonep = force.second.m_rhsExprp->cloneTreePure(false);

            // Create: __VforceEnID & RHSID (or conditional for non-ranged)
            if (ForceState::isRangedDType(lhsp)) {

                AstNodeExpr* const forceTermp
                    = new AstAnd{originalRefp->fileline(), lhsForceEnp, rhsClonep};
                if (forceExprp) {
                    forceExprp = new AstOr{originalRefp->fileline(), forceExprp, forceTermp};
                } else {
                    forceExprp = forceTermp;
                }

                // Accumulate OR of enables: __VforceEn0 | __VforceEn1 | ...
                AstNodeExpr* const lhsNegationp = lhsp->cloneTreePure(false);
                if (notEnablesp) {
                    notEnablesp->lhsp(new AstOr{originalRefp->fileline(),
                                                notEnablesp->lhsp()->unlinkFrBack(),
                                                lhsNegationp});
                } else {
                    notEnablesp = new AstNot{originalRefp->fileline(), lhsNegationp};
                }

                transformVarScopes(lhsNegationp, enVscp);
            } else {
                if (forceExprp) {
                    forceExprp = new AstCond{originalRefp->fileline(), lhsForceEnp, rhsClonep,
                                             forceExprp};
                } else {
                    AstVarRef* origRefp = originalRefp->cloneTreePure(false);
                    ForceState::markNonReplaceable(origRefp);
                    forceExprp
                        = new AstCond{originalRefp->fileline(), lhsForceEnp, rhsClonep, origRefp};
                }
            }

            transformVarScopes(lhsForceEnp, enVscp);
        }

        if (ForceState::isRangedDType(lhsp)) {
            // Create the final expression: forceExpr | ~(enablesOrExpr) & original
            return new AstOr{
                originalRefp->fileline(), forceExprp,
                new AstAnd{originalRefp->fileline(), notEnablesp, lhsp->cloneTreePure(false)}};
        }
        return forceExprp;
    }

private:
    // NODE STATE
    //  AstVarRef::user1      -> Flag indicating not to replace reference
    const VNUser1InUse m_user1InUse;
    std::unordered_map<AstVar*, std::unordered_map<AstAssignForce*, ForceInfo>>
        m_forceInfo;  // Map force statements to their ForceInfo

public:
    // CONSTRUCTORS
    ForceState() = default;
    VL_UNCOPYABLE(ForceState);

    // STATIC METHODS
    static bool isRangedDType(AstNode* nodep) {
        // If ranged we need a multibit enable to support bit-by-bit part-select forces,
        // otherwise forcing a real or other opaque dtype and need a single bit enable.
        const AstBasicDType* const basicp = nodep->dtypep()->skipRefp()->basicp();
        return basicp && basicp->isRanged();
    }
    static bool isNotReplaceable(const AstVarRef* const nodep) { return nodep->user1(); }
    static void markNonReplaceable(AstVarRef* const nodep) { nodep->user1SetOnce(); }

    static AstVarRef* getOneVarRef(AstNodeExpr* forceStmtp) {
        AstVarRef* varRefp = nullptr;
        forceStmtp->foreach([&varRefp](AstVarRef* const refp) {
            UASSERT_OBJ(!varRefp, refp,
                        "Unsupported: multiple VarRefs on LHS of force assignment");
            varRefp = refp;
        });
        UASSERT_OBJ(varRefp, forceStmtp, "`force` assignment has no VarRef on LHS");
        return varRefp;
    }

    void addForceAssignment(AstVar* varp, AstVarScope* vscp, AstNodeExpr* rhsExprp,
                            AstAssignForce* forceStmtp) {
        AstVar* const enVarp = new AstVar{
            varp->fileline(), VVarType::VAR,
            varp->name() + "__VforceEn" + std::to_string(m_forceInfo[varp].size()),
            (ForceState::isRangedDType(varp) ? varp->dtypep() : varp->findBitDType())};
        varp->addNextHere(enVarp);
        AstScope* const scopep = vscp->scopep();
        AstVarScope* const enVscp = new AstVarScope{varp->fileline(), scopep, enVarp};
        scopep->addVarsp(enVscp);

        FileLine* const flp = vscp->fileline();
        //Zero the force enable signal initially
        AstVarRef* const lhsp = new AstVarRef{flp, enVscp, VAccess::WRITE};
        AstAssign* const assignp
            = new AstAssign{flp, lhsp, makeZeroConst(enVscp, enVscp->width())};
        AstActive* const activep = new AstActive{
            flp, "force-init", new AstSenTree{flp, new AstSenItem{flp, AstSenItem::Static{}}}};
        activep->senTreeStorep(activep->sentreep());
        activep->addStmtsp(new AstInitial{flp, assignp});
        scopep->addBlocksp(activep);

        m_forceInfo[varp].emplace(forceStmtp, ForceInfo{rhsExprp, enVscp});
    }

    ForceInfo getForceInfo(AstAssignForce* forceStmtp) const {
        AstVar* varp = getOneVarRef(forceStmtp->lhsp())->varp();
        UASSERT_OBJ(varp, forceStmtp, "VarRef has null varp");
        auto it = m_forceInfo.find(varp);
        UASSERT(it != m_forceInfo.end(), "Force assignments not found");
        auto it2 = it->second.find(forceStmtp);
        UASSERT(it2 != it->second.end(), "Force info not found");
        return it2->second;
    }

    std::unordered_map<AstAssignForce*, ForceInfo> getForceInfos(AstVar* varp) const {
        auto it = m_forceInfo.find(varp);
        if (it == m_forceInfo.end()) { return std::unordered_map<AstAssignForce*, ForceInfo>{}; }
        return it->second;
    }

    void cleanupRhsExpressions() {
        for (auto& varEntry : m_forceInfo) {
            for (auto& forceEntry : varEntry.second) {
                if (forceEntry.second.m_rhsExprp) {
                    VL_DO_DANGLING(forceEntry.second.m_rhsExprp->deleteTree(),
                                   forceEntry.second.m_rhsExprp);
                }
            }
        }
    }
};

class ForceDiscoveryVisitor final : public VNVisitorConst {
    // STATE
    ForceState& m_state;
    // VISITORS
    void visit(AstAssignForce* nodep) override {
        UINFO(2, "Discovering force statement: " << nodep);

        AstVarRef* lhsVarRefp = m_state.getOneVarRef(nodep->lhsp());
        AstVar* forcedVarp = lhsVarRefp->varp();
        UASSERT(forcedVarp, "VarRef missing Varp");
        AstNodeExpr* rhsClonep = nodep->rhsp()->cloneTreePure(false);
        if (const AstSel* const selp = VN_CAST(nodep->lhsp(), Sel)) {
            UASSERT_OBJ(VN_IS(selp->lsbp(), Const), nodep->lhsp(),
                        "Unsupported: force on non-const range select");
            if (selp->lsbConst() != 0)
                rhsClonep = new AstConcat{nodep->fileline(), rhsClonep,
                                          ForceState::makeZeroConst(rhsClonep, selp->lsbConst())};
        }

        m_state.addForceAssignment(lhsVarRefp->varp(), lhsVarRefp->varScopep(), rhsClonep, nodep);

        UINFO(3, "Force assignment discovered for variable " << forcedVarp->name());
    }

    void visit(AstVarScope* nodep) override {
        // If this signal is marked externally forceable, create the public force signals
        if (nodep->varp()->isForceable()) m_state.addExternalForce(nodep, nodep->varp());
        iterateChildrenConst(nodep);
    }

    void visit(AstNode* nodep) override { iterateChildrenConst(nodep); }

public:
    // CONSTRUCTOR
    explicit ForceDiscoveryVisitor(AstNetlist* nodep, ForceState& state)
        : m_state{state} {
        iterateAndNextConstNull(nodep->modulesp());
    }
};

class ForceConvertVisitor final : public VNVisitor {
    // STATE
    ForceState& m_state;

    // VISITORS
    void visit(AstAssignForce* nodep) override {
        UINFO(2, "Converting force statement: " << nodep);

        // Create statements to implement the force logic:
        // 1. AND all other __VforceEnID with negation of current __VforceEnID
        // 2. Set current __VforceEnID to all ones (on the given range)

        // Create the range for forcing (for now, assume full variable)
        FileLine* flp = nodep->fileline();
        AstNodeExpr* const lhsp = nodep->lhsp();  // The LValue we are forcing
        AstVar* const forcedVarp = m_state.getOneVarRef(lhsp)->varp();
        UASSERT_OBJ(forcedVarp, lhsp, "VarRef missing Varp");
        V3Number ones{lhsp, ForceState::isRangedDType(lhsp) ? lhsp->width() : 1};
        ones.setAllBits1();
        AstNodeExpr* currentEnAllOnesp = new AstConst{flp, ones};
        // Get the enable variable for current force ID
        ForceState::ForceInfo currectForceInfo = m_state.getForceInfo(nodep);
        AstVarScope* const currentEnVscp = currectForceInfo.m_enVscp;
        // 1. Set current __VforceEnID to all ones
        AstAssign* const enableCurrentp
            = new AstAssign{flp, lhsp->cloneTreePure(false), currentEnAllOnesp};
        ForceState::transformVarScopes(enableCurrentp->lhsp(), currentEnVscp);
        AstNode* stmtListp = enableCurrentp;

        // 2. AND all other __VforceEnID with negation of current range
        for (const auto& force : m_state.getForceInfos(forcedVarp)) {
            if (force.second.m_enVscp == currectForceInfo.m_enVscp)
                continue;  // Skip current force

            AstNodeExpr* lhsCopyp = lhsp->cloneTreePure(false);
            AstNodeExpr* negationExprp = new AstAnd{
                flp, lhsCopyp, new AstNot{flp, currentEnAllOnesp->cloneTreePure(false)}};
            AstVarScope* const otherEnVscp = force.second.m_enVscp;
            ForceState::transformVarScopes(lhsCopyp, otherEnVscp);

            AstAssign* const disableOtherp
                = new AstAssign{flp, lhsp->cloneTreePure(false), negationExprp};

            ForceState::transformVarScopes(disableOtherp->lhsp(), otherEnVscp);

            stmtListp->addNext(disableOtherp);
        }

        UINFO(3, "Replaced force with enable logic: " << currectForceInfo.m_enVscp->name());

        // Replace the force statement with our statements

        UASSERT_OBJ(stmtListp, nodep, "Empty statement list");
        nodep->replaceWith(stmtListp);
        VL_DO_DANGLING(pushDeletep(nodep), nodep);
    }

    void visit(AstRelease* nodep) override {
        UINFO(2, "Converting release statement: " << nodep);
        AstNodeExpr* const lhsp = nodep->lhsp();  // The LValue we are releasing
        AstVarRef* lhsVarRefp = m_state.getOneVarRef(lhsp);

        AstVar* const releasedVarp = lhsVarRefp->varp();

        if (!releasedVarp) {
            VL_DO_DANGLING(pushDeletep(nodep->unlinkFrBack()), nodep);
            return;
        }

        const std::unordered_map<AstAssignForce*, ForceState::ForceInfo>& forces
            = m_state.getForceInfos(releasedVarp);

        if (forces.empty()) {
            VL_DO_DANGLING(pushDeletep(nodep->unlinkFrBack()), nodep);
            return;
        }

        FileLine* flp = nodep->fileline();
        AstNode* stmtListp = nullptr;
        // If the variable is not continuously assigned, update its real value to current
        // forced value
        if (!releasedVarp->isContinuously()) {
            // Create force read expression
            stmtListp = new AstAssign{flp, lhsp->cloneTreePure(false),
                                      m_state.createForceReadExpression(forces, lhsVarRefp, lhsp)};
        }

        // AND all __VforceEnIDs with negation of released range (release all forces)

        for (const auto& force : forces) {
            AstVarScope* const enVscp = force.second.m_enVscp;

            AstNodeExpr* copyForDisablep = lhsp->cloneTreePure(false);
            AstNodeExpr* copyForAssignLHSp = lhsp->cloneTreePure(false);
            if (!ForceState::isRangedDType(releasedVarp)) {
                // Assign bit dtype, so if the var is for ex. real type, it becomes a bit
                copyForDisablep->dtypep(copyForDisablep->findBitDType());
                copyForAssignLHSp->dtypep(copyForAssignLHSp->findBitDType());
            }

            AstNodeExpr* disableExprp = new AstAnd{
                flp, copyForDisablep,
                ForceState::makeZeroConst(
                    lhsp, ForceState::isRangedDType(releasedVarp) ? lhsp->width() : 1)};

            AstAssign* const disableAssignp = new AstAssign{flp, copyForAssignLHSp, disableExprp};

            ForceState::transformVarScopes(copyForDisablep, enVscp);

            ForceState::transformVarScopes(copyForAssignLHSp, enVscp);

            if (!stmtListp) {
                stmtListp = disableAssignp;
            } else {
                stmtListp->addNext(disableAssignp);
            }
        }

        // Replace the release statement
        nodep->replaceWith(stmtListp);
        VL_DO_DANGLING(pushDeletep(nodep), nodep);

        UINFO(3, "Replaced release with disable logic");
    }

    void visit(AstNode* nodep) override { iterateChildren(nodep); }

public:
    // CONSTRUCTOR
    // cppcheck-suppress constParameterCallback
    ForceConvertVisitor(AstNetlist* nodep, ForceState& state)
        : m_state{state} {
        iterateAndNextNull(nodep->modulesp());
    }
};

class ForceReplaceVisitor final : public VNVisitor {
    // STATE
    const ForceState& m_state;

    void visit(AstVarRef* nodep) override {
        if (ForceState::isNotReplaceable(nodep)) return;

        AstVar* const varp = nodep->varp();
        const std::unordered_map<AstAssignForce*, ForceState::ForceInfo>& forces
            = m_state.getForceInfos(varp);  // Get the force info for this variable
        if (forces.empty()) return;

        if (nodep->access().isRW()) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: Signals used via read-write reference cannot be forced");
        } else if (nodep->access().isReadOnly()) {
            // Check if this variable has force components
            ForceState::markNonReplaceable(nodep);
            nodep->replaceWith(m_state.createForceReadExpression(forces, nodep, nodep));
            VL_DO_DANGLING(pushDeletep(nodep), nodep);
        }
    }
    void visit(AstNode* nodep) override { iterateChildren(nodep); }

public:
    // CONSTRUCTOR
    explicit ForceReplaceVisitor(AstNetlist* nodep, const ForceState& state)
        : m_state{state} {
        iterateAndNextNull(nodep->modulesp());
    }
};
//######################################################################
//

void V3Force::forceAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    if (!v3Global.hasForceableSignals()) return;
    {
        ForceState state;
        {
            ForceDiscoveryVisitor{nodep, state};
        }  // Stage 1: Discover forces and create all __VforceEnID signals
        { ForceConvertVisitor{nodep, state}; }  // Stage 2: Replace force/release statements
        { ForceReplaceVisitor{nodep, state}; }  // Stage 3: Replace variable reads
        state.cleanupRhsExpressions();
        V3Global::dumpCheckGlobalTree("force", 0, dumpTreeEitherLevel() >= 3);
    }
}
