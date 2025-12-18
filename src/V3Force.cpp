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
//          - add an extra signal array: <name>__VforceEns[ID] and set it on the force range to
//            ones
//          - add an initial assignment: initial <name>__VforceEns[ID] = 0;
//          - store the <RHS[ID]> in a scope
//      Replace all READ references to <name> with the following expression (force read
//      expression):
//          <name>__VforceEns[0] & <RHS0> | ... | <name>__VforceEns[LastID] & <LastRHS>
//          | ~(<name>__VforceEns[0] | ... | <name>__VforceEns[LastID]) & <name>
//          (or the same with if-else for non-ranged datatypes)
//
//  Replace each AstAssignForce with:
//      - <lhs>__VforceEns[ID] = all ones (on the given range)
//      - for every [ID2] other than[ID], do: <lhs>__VforceEns[ID2] &= ~(<lhs>__VforceEns[ID])
//          (or just single-bit 1 for non-ranged datatypes)
//
//  Replace each AstRelease with:
//      - <lhs> = (force read expression of <lhs>) // iff lhs is not a net
//      - For each [ID]: <lhs>__VforceEns[ID] &= all zeros (on the given range)
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
        AstVarScope* m_enArrayVscp = nullptr;  // Force enabled array scope for this ID
                                               // (__VforceEns[ID])
        AstVarScope* m_rhsArrayVscp = nullptr;  // RHS storage array scope (__VforceRHS[ID])
        size_t m_forceId = 0;  // Force index for this signal

        ForceInfo(AstNodeExpr* rhsExprp, size_t forceId)
            : m_rhsExprp{rhsExprp}
            , m_forceId{forceId} {}
    };

    struct ForceVarInfo final {
        std::unordered_map<AstVarScope*, std::unordered_map<AstAssignForce*, ForceInfo>>
            m_scopeForces;  // Force statements grouped by the var scope they affect
        size_t m_maxForces = 0;  // Maximum number of forces seen on this var across scopes
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

    static AstNodeExpr* createEnableExpr(const ForceState::ForceInfo& info, AstNodeExpr* modelp) {
        AstNodeExpr* exprp = modelp->cloneTreePure(false);
        const auto replaceRef = [&info](AstNodeVarRef* refp) -> AstNodeExpr* {
            UASSERT_OBJ(info.m_enArrayVscp, refp, "Force enable storage missing");
            AstArraySel* const selp = new AstArraySel{
                refp->fileline(),
                new AstVarRef{refp->fileline(), info.m_enArrayVscp, refp->access()},
                static_cast<int>(info.m_forceId)};
            if (refp->backp()) {
                refp->replaceWith(selp);
                VL_DO_DANGLING(refp->deleteTree(), refp);
                return nullptr;
            }
            VL_DO_DANGLING(refp->deleteTree(), refp);
            return selp;
        };

        if (AstVarRef* const rootRefp = VN_CAST(exprp, VarRef)) {
            exprp = replaceRef(rootRefp);
        } else {
            exprp->foreach([&replaceRef](AstNodeVarRef* refp) { replaceRef(refp); });
        }
        return exprp;
    }

    // Creates an enable expression using a variable index for array selection (used in loops)
    static AstNodeExpr* createEnableExprWithVarIndex(AstVarScope* enArrayVscp, AstNodeExpr* modelp,
                                                     AstVarScope* indexVscp, VAccess access) {
        AstNodeExpr* exprp = modelp->cloneTreePure(false);
        const auto replaceRef
            = [enArrayVscp, indexVscp, access](AstNodeVarRef* refp) -> AstNodeExpr* {
            UASSERT_OBJ(enArrayVscp, refp, "Force enable storage missing");
            AstArraySel* const selp = new AstArraySel{
                refp->fileline(), new AstVarRef{refp->fileline(), enArrayVscp, access},
                new AstVarRef{refp->fileline(), indexVscp, VAccess::READ}};
            if (refp->backp()) {
                refp->replaceWith(selp);
                VL_DO_DANGLING(refp->deleteTree(), refp);
                return nullptr;
            }
            VL_DO_DANGLING(refp->deleteTree(), refp);
            return selp;
        };

        if (AstVarRef* const rootRefp = VN_CAST(exprp, VarRef)) {
            exprp = replaceRef(rootRefp);
        } else {
            exprp->foreach([&replaceRef](AstNodeVarRef* refp) { replaceRef(refp); });
        }
        return exprp;
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
        // __VforceEns[0] & RHS0 | __VforceEns[1] & RHS1 | ... |
        // ~(__VforceEns[0] | __VforceEns[1] | ...) & original

        // Build the force terms: __VforceEns[ID] & RHSID
        AstNodeExpr* forceExprp = nullptr;
        AstNot* notEnablesp = nullptr;
        for (const auto& force : forces) {
            const ForceState::ForceInfo& forceInfo = force.second;
            AstNodeExpr* const lhsForceEnp = ForceState::createEnableExpr(forceInfo, lhsp);
            AstNodeExpr* rhsClonep = force.second.m_rhsExprp->cloneTreePure(false);

            // Create: __VforceEns[ID] & RHSID (or conditional for non-ranged)
            if (ForceState::isRangedDType(lhsp)) {

                AstNodeExpr* const forceTermp
                    = new AstAnd{originalRefp->fileline(), lhsForceEnp, rhsClonep};
                if (forceExprp) {
                    forceExprp = new AstOr{originalRefp->fileline(), forceExprp, forceTermp};
                } else {
                    forceExprp = forceTermp;
                }

                // Accumulate OR of enables: __VforceEns[0] | __VforceEns[1] | ...
                AstNodeExpr* const lhsNegationp = ForceState::createEnableExpr(forceInfo, lhsp);
                if (notEnablesp) {
                    notEnablesp->lhsp(new AstOr{originalRefp->fileline(),
                                                notEnablesp->lhsp()->unlinkFrBack(),
                                                lhsNegationp});
                } else {
                    notEnablesp = new AstNot{originalRefp->fileline(), lhsNegationp};
                }
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
    std::unordered_map<AstVar*, ForceVarInfo>
        m_forceInfo;  // Map vars to per-scope force statements

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
        ForceVarInfo& varInfo = m_forceInfo[varp];
        auto& forceMap = varInfo.m_scopeForces[vscp];
        const size_t forceId = forceMap.size();
        forceMap.emplace(forceStmtp, ForceInfo{rhsExprp, forceId});
        if (varInfo.m_maxForces < forceId + 1) varInfo.m_maxForces = forceId + 1;
    }

    ForceInfo getForceInfo(AstAssignForce* forceStmtp) const {
        AstVarRef* varRefp = getOneVarRef(forceStmtp->lhsp());
        AstVar* varp = varRefp->varp();
        AstVarScope* vscp = varRefp->varScopep();
        UASSERT_OBJ(varp, forceStmtp, "VarRef has null varp");
        UASSERT_OBJ(vscp, forceStmtp, "VarRef has null varScopep");
        auto varIt = m_forceInfo.find(varp);
        UASSERT(varIt != m_forceInfo.end(), "Force assignments not found");
        auto scopeIt = varIt->second.m_scopeForces.find(vscp);
        UASSERT(scopeIt != varIt->second.m_scopeForces.end(),
                "Force assignments for varScope not found");
        auto forceIt = scopeIt->second.find(forceStmtp);
        UASSERT(forceIt != scopeIt->second.end(), "Force info not found");
        return forceIt->second;
    }

    std::unordered_map<AstAssignForce*, ForceInfo> getForceInfos(AstVarScope* vscp) const {
        if (!vscp) return std::unordered_map<AstAssignForce*, ForceInfo>{};
        auto varIt = m_forceInfo.find(vscp->varp());
        if (varIt == m_forceInfo.end()) {
            return std::unordered_map<AstAssignForce*, ForceInfo>{};
        }
        auto scopeIt = varIt->second.m_scopeForces.find(vscp);
        if (scopeIt == varIt->second.m_scopeForces.end()) {
            return std::unordered_map<AstAssignForce*, ForceInfo>{};
        }
        return scopeIt->second;
    }

    size_t getNumForces(AstVar* varp) const {
        auto varIt = m_forceInfo.find(varp);
        if (varIt == m_forceInfo.end()) return 0;
        return varIt->second.m_maxForces;
    }

    void createEnStorage(std::unordered_map<AstScope*, AstVarScope*>& loopVarScopes) {
        for (auto& varEntry : m_forceInfo) {
            AstVar* const varp = varEntry.first;
            auto& varInfo = varEntry.second;
            if (varInfo.m_scopeForces.empty()) continue;
            const size_t numForces = varInfo.m_maxForces;
            if (!numForces) continue;
            FileLine* const flp = varp->fileline();
            AstRange* const range = new AstRange{flp, static_cast<int>(numForces - 1), 0};
            AstNodeDType* const elementDTypep
                = ForceState::isRangedDType(varp) ? varp->dtypep() : varp->findBitDType();
            AstUnpackArrayDType* const enArrayDTypep
                = new AstUnpackArrayDType{flp, elementDTypep, range};
            v3Global.rootp()->typeTablep()->addTypesp(enArrayDTypep);
            AstVar* const enVarp
                = new AstVar{flp, VVarType::VAR, varp->name() + "__VforceEns", enArrayDTypep};
            enVarp->trace(false);
            enVarp->sigUserRWPublic(true);
            varp->addNextHere(enVarp);
            for (auto& scopeEntry : varInfo.m_scopeForces) {
                AstScope* const scopep = scopeEntry.first->scopep();
                AstVarScope* const enVscp = new AstVarScope{flp, scopep, enVarp};
                scopep->addVarsp(enVscp);
                for (auto& forceEntry : scopeEntry.second) {
                    ForceInfo& finfo = forceEntry.second;
                    finfo.m_enArrayVscp = enVscp;
                }
                AstVarScope* loopVscp = nullptr;
                auto it = loopVarScopes.find(scopep);
                if (it != loopVarScopes.end()) {
                    loopVscp = it->second;
                } else {
                    AstVar* const loopVarp = new AstVar{flp, VVarType::BLOCKTEMP, "__Vforcei",
                                                        varp->findSigned32DType()};
                    loopVarp->funcLocal(false);
                    loopVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
                    scopep->modp()->addStmtsp(loopVarp);
                    loopVscp = new AstVarScope{flp, scopep, loopVarp};
                    scopep->addVarsp(loopVscp);
                    loopVarScopes[scopep] = loopVscp;
                }

                // Create loop: for (__Vi = 0; __Vi < numForces; __Vi++)
                AstNode* loopStmtsp = new AstAssign{
                    flp, new AstVarRef{flp, loopVscp, VAccess::WRITE}, new AstConst{flp, 0}};

                AstLoop* const loopp = new AstLoop{flp};
                loopStmtsp->addNext(loopp);

                // Loop condition: __Vi < numForces
                loopp->addStmtsp(new AstLoopTest{
                    flp, loopp,
                    new AstLt{flp, new AstVarRef{flp, loopVscp, VAccess::READ},
                              new AstConst{flp, static_cast<uint32_t>(numForces)}}});

                // Loop body: __VforceEns[__Vi] = 0
                AstNodeExpr* const modelLhsp = scopeEntry.second.begin()->first->lhsp();
                AstNodeExpr* const lhsForLoop = ForceState::createEnableExprWithVarIndex(
                    enVscp, modelLhsp, loopVscp, VAccess::WRITE);
                AstAssign* const zeroAssignp = new AstAssign{
                    flp, lhsForLoop, makeZeroConst(lhsForLoop, lhsForLoop->width())};
                loopp->addStmtsp(zeroAssignp);

                // Increment: __Vi++
                loopp->addStmtsp(
                    new AstAssign{flp, new AstVarRef{flp, loopVscp, VAccess::WRITE},
                                  new AstAdd{flp, new AstVarRef{flp, loopVscp, VAccess::READ},
                                             new AstConst{flp, 1}}});

                AstActive* const activep = new AstActive{
                    flp, "force-init",
                    new AstSenTree{flp, new AstSenItem{flp, AstSenItem::Static{}}}};
                activep->senTreeStorep(activep->sentreep());
                activep->addStmtsp(new AstInitial{flp, loopStmtsp});
                scopep->addBlocksp(activep);
            }
        }
    }

    void createRhsStorage() {
        for (auto& varEntry : m_forceInfo) {
            AstVar* const varp = varEntry.first;
            auto& varInfo = varEntry.second;
            if (varInfo.m_scopeForces.empty()) continue;
            const size_t numForces = varInfo.m_maxForces;
            if (!numForces) continue;
            FileLine* const flp = varp->fileline();
            AstRange* const range = new AstRange{flp, static_cast<int>(numForces - 1), 0};
            AstUnpackArrayDType* const rhsArrayDTypep
                = new AstUnpackArrayDType{flp, varp->dtypep(), range};
            v3Global.rootp()->typeTablep()->addTypesp(rhsArrayDTypep);
            AstVar* const rhsVarp
                = new AstVar{flp, VVarType::VAR, varp->name() + "__VforceRHS", rhsArrayDTypep};
            rhsVarp->isInternal(true);
            rhsVarp->trace(false);
            rhsVarp->sigUserRWPublic(true);
            varp->addNextHere(rhsVarp);
            for (auto& scopeEntry : varInfo.m_scopeForces) {
                AstScope* const scopep = scopeEntry.first->scopep();
                AstVarScope* const rhsVscp = new AstVarScope{flp, scopep, rhsVarp};
                scopep->addVarsp(rhsVscp);
                for (auto& forceEntry : scopeEntry.second) {
                    ForceInfo& finfo = forceEntry.second;
                    finfo.m_rhsArrayVscp = rhsVscp;
                    UASSERT_OBJ(finfo.m_rhsExprp, varp, "Missing force RHS expression");

                    // Build sensitivity list from all VarRefs in the RHS expression
                    AstSenItem* senItemsp = nullptr;
                    finfo.m_rhsExprp->foreach([&](AstVarRef* refp) {
                        AstSenItem* const itemp
                            = new AstSenItem{flp, VEdgeType::ET_CHANGED,
                                             new AstVarRef{flp, refp->varScopep(), VAccess::READ}};
                        if (senItemsp) {
                            senItemsp->addNext(itemp);
                        } else {
                            senItemsp = itemp;
                        }
                    });

                    // Always create initial assignment: initial __VforceRHS[ID] = rhsExpr
                    {
                        AstVarRef* const initLhsRefp = new AstVarRef{flp, rhsVscp, VAccess::WRITE};
                        AstNodeExpr* const initLhsSelp
                            = new AstArraySel{flp, initLhsRefp, static_cast<int>(finfo.m_forceId)};
                        AstAssign* const initAssignp = new AstAssign{
                            flp, initLhsSelp, finfo.m_rhsExprp->cloneTreePure(false)};
                        AstActive* const initActivep = new AstActive{
                            flp, "force-rhs-init",
                            new AstSenTree{flp, new AstSenItem{flp, AstSenItem::Static{}}}};
                        initActivep->senTreeStorep(initActivep->sentreep());
                        initActivep->addStmtsp(new AstInitial{flp, initAssignp});
                        scopep->addBlocksp(initActivep);
                    }

                    // Only create always block if there are VarRefs in the RHS
                    if (senItemsp) {
                        AstVarRef* const lhsRefp = new AstVarRef{flp, rhsVscp, VAccess::WRITE};
                        AstNodeExpr* const lhsSelp
                            = new AstArraySel{flp, lhsRefp, static_cast<int>(finfo.m_forceId)};
                        AstAssign* const assignp
                            = new AstAssign{flp, lhsSelp, finfo.m_rhsExprp->cloneTreePure(false)};

                        // Create always block with sensitivity list
                        AstSenTree* const sentreep = new AstSenTree{flp, senItemsp};
                        AstActive* const activep = new AstActive{flp, "force-rhs", sentreep};
                        activep->senTreeStorep(activep->sentreep());
                        activep->addStmtsp(
                            new AstAlways{flp, VAlwaysKwd::ALWAYS, nullptr, assignp});
                        scopep->addBlocksp(activep);
                    }
                }
            }
        }
    }

    void cleanupRhsExpressions() {
        for (auto& varEntry : m_forceInfo) {
            for (auto& scopeEntry : varEntry.second.m_scopeForces) {
                for (auto& forceEntry : scopeEntry.second) {
                    if (forceEntry.second.m_rhsExprp) {
                        VL_DO_DANGLING(forceEntry.second.m_rhsExprp->deleteTree(),
                                       forceEntry.second.m_rhsExprp);
                    }
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
            const int lsb = selp->lsbConst();
            const int selWidth = selp->widthConst();
            const int varWidth = lhsVarRefp->width();
            const int upperPad = varWidth - (lsb + selWidth);
            if (upperPad > 0) {
                rhsClonep = new AstConcat{nodep->fileline(),
                                          ForceState::makeZeroConst(nodep, upperPad), rhsClonep};
            }
            if (lsb != 0) {
                rhsClonep = new AstConcat{nodep->fileline(), rhsClonep,
                                          ForceState::makeZeroConst(nodep, lsb)};
            }
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
    std::unordered_map<AstScope*, AstVarScope*>& m_loopVarScopes;

    // Get or create the loop variable for a given scope
    AstVarScope* getOrCreateLoopVarScope(AstScope* scopep, FileLine* flp, AstNode* dtypeSourcep) {
        auto it = m_loopVarScopes.find(scopep);
        if (it != m_loopVarScopes.end()) return it->second;

        AstVar* const loopVarp
            = new AstVar{flp, VVarType::BLOCKTEMP, "__Vforcei", dtypeSourcep->findSigned32DType()};
        loopVarp->funcLocal(false);
        loopVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        scopep->modp()->addStmtsp(loopVarp);
        AstVarScope* const loopVscp = new AstVarScope{flp, scopep, loopVarp};
        scopep->addVarsp(loopVscp);

        m_loopVarScopes[scopep] = loopVscp;
        return loopVscp;
    }

    // VISITORS
    void visit(AstAssignForce* nodep) override {
        UINFO(2, "Converting force statement: " << nodep);

        // Create statements to implement the force logic:
        // 1. Set current __VforceEns[ID] to all ones (on the given range)
        // 2. For all other IDs, AND __VforceEns[ID] with negation of current enable using a loop

        // Create the range for forcing (for now, assume full variable)
        FileLine* flp = nodep->fileline();
        AstNodeExpr* const lhsp = nodep->lhsp();  // The LValue we are forcing
        AstVarRef* const lhsVarRefp = m_state.getOneVarRef(lhsp);
        AstVar* const forcedVarp = lhsVarRefp->varp();
        AstVarScope* const forceVscp = lhsVarRefp->varScopep();
        UASSERT_OBJ(forcedVarp, lhsp, "VarRef missing Varp");
        UASSERT_OBJ(forceVscp, lhsp, "VarRef missing VarScope");
        V3Number ones{lhsp, ForceState::isRangedDType(lhsp) ? lhsp->width() : 1};
        ones.setAllBits1();
        AstNodeExpr* currentEnAllOnesp = new AstConst{flp, ones};
        // Get the enable variable for current force ID
        ForceState::ForceInfo currentForceInfo = m_state.getForceInfo(nodep);
        // 1. Set current __VforceEns[ID] to all ones
        AstAssign* const enableCurrentp = new AstAssign{
            flp, ForceState::createEnableExpr(currentForceInfo, lhsp), currentEnAllOnesp};
        AstNode* stmtListp = enableCurrentp;

        // 2. AND all other __VforceEns[ID] with negation of current range using a loop
        const size_t numForces = m_state.getNumForces(forcedVarp);
        if (numForces > 1) {
            // Get or create the shared loop variable for this scope
            AstScope* const scopep = forceVscp->scopep();
            AstVarScope* const loopVscp = getOrCreateLoopVarScope(scopep, flp, forcedVarp);

            // Create loop: for (__Vi = 0; __Vi < numForces; __Vi++)
            AstNode* loopStmtsp = new AstAssign{flp, new AstVarRef{flp, loopVscp, VAccess::WRITE},
                                                new AstConst{flp, 0}};

            AstLoop* const loopp = new AstLoop{flp};
            loopStmtsp->addNext(loopp);

            // Loop condition: __Vi < numForces
            loopp->addStmtsp(
                new AstLoopTest{flp, loopp,
                                new AstLt{flp, new AstVarRef{flp, loopVscp, VAccess::READ},
                                          new AstConst{flp, static_cast<uint32_t>(numForces)}}});

            // Loop body: if (__Vi != currentForceId) { __VforceEns[__Vi] &= ~mask; }
            AstNodeExpr* const lhsForLoop = ForceState::createEnableExprWithVarIndex(
                currentForceInfo.m_enArrayVscp, lhsp, loopVscp, VAccess::WRITE);
            AstNodeExpr* const rhsForLoopRead = ForceState::createEnableExprWithVarIndex(
                currentForceInfo.m_enArrayVscp, lhsp, loopVscp, VAccess::READ);
            AstNodeExpr* const negationExprp = new AstAnd{
                flp, rhsForLoopRead, new AstNot{flp, currentEnAllOnesp->cloneTreePure(false)}};

            AstAssign* const disableOtherp = new AstAssign{flp, lhsForLoop, negationExprp};

            // Condition: if (__Vi != currentForceId)
            AstIf* const ifNotCurrentp = new AstIf{
                flp,
                new AstNeq{flp, new AstVarRef{flp, loopVscp, VAccess::READ},
                           new AstConst{flp, static_cast<uint32_t>(currentForceInfo.m_forceId)}},
                disableOtherp, nullptr};

            loopp->addStmtsp(ifNotCurrentp);

            // Increment: __Vi++
            loopp->addStmtsp(
                new AstAssign{flp, new AstVarRef{flp, loopVscp, VAccess::WRITE},
                              new AstAdd{flp, new AstVarRef{flp, loopVscp, VAccess::READ},
                                         new AstConst{flp, 1}}});

            stmtListp->addNext(loopStmtsp);
        }

        UINFO(3, "Replaced force with enable logic: " << forcedVarp->name());

        // Replace the force statement with our statements

        UASSERT_OBJ(stmtListp, nodep, "Empty statement list");
        nodep->replaceWith(stmtListp);
        VL_DO_DANGLING(pushDeletep(nodep), nodep);
    }

    void visit(AstRelease* nodep) override {
        UINFO(2, "Converting release statement: " << nodep);
        AstNodeExpr* const lhsp = nodep->lhsp();  // The LValue we are releasing
        AstVarRef* lhsVarRefp = m_state.getOneVarRef(lhsp);
        AstVarScope* const releaseVscp = lhsVarRefp->varScopep();

        AstVar* const releasedVarp = lhsVarRefp->varp();

        if (!releasedVarp) {
            VL_DO_DANGLING(pushDeletep(nodep->unlinkFrBack()), nodep);
            return;
        }

        const std::unordered_map<AstAssignForce*, ForceState::ForceInfo>& forces
            = m_state.getForceInfos(releaseVscp);

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

        // AND all __VforceEns[ID] with negation of released range (release all forces)

        for (const auto& force : forces) {
            AstNodeExpr* copyForDisablep = ForceState::createEnableExpr(force.second, lhsp);
            AstNodeExpr* copyForAssignLHSp = ForceState::createEnableExpr(force.second, lhsp);
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
    ForceConvertVisitor(AstNetlist* nodep, ForceState& state,
                        std::unordered_map<AstScope*, AstVarScope*>& loopVarScopes)
        : m_state{state}
        , m_loopVarScopes{loopVarScopes} {
        iterateAndNextNull(nodep->modulesp());
    }
};

class ForceReplaceVisitor final : public VNVisitor {
    // STATE
    const ForceState& m_state;

    void visit(AstVarRef* nodep) override {
        if (ForceState::isNotReplaceable(nodep)) return;

        AstVarScope* const vscp = nodep->varScopep();
        const std::unordered_map<AstAssignForce*, ForceState::ForceInfo>& forces
            = m_state.getForceInfos(vscp);  // Get the force info for this variable
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
        std::unordered_map<AstScope*, AstVarScope*> loopVarScopes;
        {
            ForceDiscoveryVisitor{nodep, state};
        }  // Stage 1: Discover forces and create all __VforceEns[ID] signals
        state.createEnStorage(loopVarScopes);  // Stage 1b: Materialize enable storage wires
        state.createRhsStorage();  // Stage 1c: Materialize RHS storage wires
        {
            ForceConvertVisitor{nodep, state, loopVarScopes};
        }  // Stage 2: Replace force/release statements
        { ForceReplaceVisitor{nodep, state}; }  // Stage 3: Replace variable reads
        state.cleanupRhsExpressions();
        V3Global::dumpCheckGlobalTree("force", 0, dumpTreeEitherLevel() >= 3);
    }
}
