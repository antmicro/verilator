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

    // Storage for force read result and negation mask per VarScope
    struct ForceReadInfo final {
        AstVarScope* m_rdVscp = nullptr;  // __VforceRd - accumulated read result
        AstVarScope* m_negVscp = nullptr;  // __VforceNeg - accumulated negation mask
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
        const auto replaceRef = [=](AstNodeVarRef* refp) -> AstNodeExpr* {
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

    // Get or create the loop variable for a given scope
    static AstVarScope* getOrCreateLoopVar(std::unordered_map<AstScope*, AstVarScope*>& loopVars,
                                           AstScope* scopep, FileLine* flp, AstNode* dtypeSrcp) {
        auto it = loopVars.find(scopep);
        if (it != loopVars.end()) return it->second;
        AstVar* const loopVarp
            = new AstVar{flp, VVarType::BLOCKTEMP, "__Vforcei", dtypeSrcp->findSigned32DType()};
        loopVarp->funcLocal(false);
        loopVarp->lifetime(VLifetime::AUTOMATIC_EXPLICIT);
        scopep->modp()->addStmtsp(loopVarp);
        AstVarScope* const loopVscp = new AstVarScope{flp, scopep, loopVarp};
        scopep->addVarsp(loopVscp);
        loopVars[scopep] = loopVscp;
        return loopVscp;
    }

    // Build loop body for force read calculation
    static void addForceReadLoopBody(AstLoop* loopp, AstVarScope* rdVscp, AstVarScope* negVscp,
                                     AstVarScope* enArrayVscp, AstVarScope* rhsArrayVscp,
                                     AstVarScope* loopVscp, bool isRanged, FileLine* flp) {
        AstNodeExpr* const enSelp
            = new AstArraySel{flp, new AstVarRef{flp, enArrayVscp, VAccess::READ},
                              new AstVarRef{flp, loopVscp, VAccess::READ}};
        AstNodeExpr* const rhsSelp
            = new AstArraySel{flp, new AstVarRef{flp, rhsArrayVscp, VAccess::READ},
                              new AstVarRef{flp, loopVscp, VAccess::READ}};

        if (isRanged) {
            // __VforceRd |= __VforceEns[i] & __VforceRHS[i]
            loopp->addStmtsp(
                new AstAssign{flp, new AstVarRef{flp, rdVscp, VAccess::WRITE},
                              new AstOr{flp, new AstVarRef{flp, rdVscp, VAccess::READ},
                                        new AstAnd{flp, enSelp, rhsSelp}}});
        } else {
            // if (__VforceEns[i]) __VforceRd = __VforceRHS[i]
            loopp->addStmtsp(new AstIf{
                flp, enSelp,
                new AstAssign{flp, new AstVarRef{flp, rdVscp, VAccess::WRITE}, rhsSelp}, nullptr});
        }

        // __VforceNeg |= __VforceEns[i]
        AstNodeExpr* const enSelp2
            = new AstArraySel{flp, new AstVarRef{flp, enArrayVscp, VAccess::READ},
                              new AstVarRef{flp, loopVscp, VAccess::READ}};
        loopp->addStmtsp(
            new AstAssign{flp, new AstVarRef{flp, negVscp, VAccess::WRITE},
                          new AstOr{flp, new AstVarRef{flp, negVscp, VAccess::READ}, enSelp2}});
    }

    // Add final assignment after force read loop
    static void addForceReadFinal(AstNode* stmtsp, AstVarScope* rdVscp, AstVarScope* negVscp,
                                  AstVarRef* origRefp, bool isRanged, FileLine* flp) {
        if (isRanged) {
            // __VforceRd |= ~__VforceNeg & realSig
            stmtsp->addNext(new AstAssign{
                flp, new AstVarRef{flp, rdVscp, VAccess::WRITE},
                new AstOr{flp, new AstVarRef{flp, rdVscp, VAccess::READ},
                          new AstAnd{flp,
                                     new AstNot{flp, new AstVarRef{flp, negVscp, VAccess::READ}},
                                     origRefp}}});
        } else {
            // if (!__VforceNeg) __VforceRd = realSig
            stmtsp->addNext(
                new AstIf{flp, new AstNot{flp, new AstVarRef{flp, negVscp, VAccess::READ}},
                          new AstAssign{flp, new AstVarRef{flp, rdVscp, VAccess::WRITE}, origRefp},
                          nullptr});
        }
    }

    static AstConst* makeZeroConst(AstNode* nodep, int width) {
        V3Number zero{nodep, width};
        zero.setAllBits0();
        return new AstConst{nodep->fileline(), zero};
    }

    // Create inline force read loop statements
    // Returns a list of statements that compute the force read value into rdVscp
    static AstNode* createForceReadLoop(
        AstVarScope* vscp, AstVarScope* rdVscp, AstVarScope* negVscp,
        const std::unordered_map<AstAssignForce*, ForceState::ForceInfo>& forces, size_t numForces,
        std::unordered_map<AstScope*, AstVarScope*>& loopVarScopes, FileLine* flp) {
        AstScope* const scopep = vscp->scopep();
        AstVar* const varp = vscp->varp();
        const bool isRanged = ForceState::isRangedDType(varp);

        AstVarScope* const loopVscp = getOrCreateLoopVar(loopVarScopes, scopep, flp, varp);
        AstVarScope* const enArrayVscp = forces.begin()->second.m_enArrayVscp;
        AstVarScope* const rhsArrayVscp = forces.begin()->second.m_rhsArrayVscp;

        // Init: __VforceRd = 0; __VforceNeg = 0; __Vforcei = 0;
        // Note: __VforceRHS is kept up-to-date by always @(*) blocks
        AstNode* stmtsp = new AstAssign{flp, new AstVarRef{flp, rdVscp, VAccess::WRITE},
                                        makeZeroConst(varp, varp->dtypep()->width())};
        stmtsp->addNext(
            new AstAssign{flp, new AstVarRef{flp, negVscp, VAccess::WRITE},
                          makeZeroConst(varp, isRanged ? varp->dtypep()->width() : 1)});
        stmtsp->addNext(new AstAssign{flp, new AstVarRef{flp, loopVscp, VAccess::WRITE},
                                      new AstConst{flp, 0}});

        AstLoop* const loopp = new AstLoop{flp};
        stmtsp->addNext(loopp);

        loopp->addStmtsp(
            new AstLoopTest{flp, loopp,
                            new AstLt{flp, new AstVarRef{flp, loopVscp, VAccess::READ},
                                      new AstConst{flp, static_cast<uint32_t>(numForces)}}});

        addForceReadLoopBody(loopp, rdVscp, negVscp, enArrayVscp, rhsArrayVscp, loopVscp, isRanged,
                             flp);

        loopp->addStmtsp(new AstAssign{
            flp, new AstVarRef{flp, loopVscp, VAccess::WRITE},
            new AstAdd{flp, new AstVarRef{flp, loopVscp, VAccess::READ}, new AstConst{flp, 1}}});

        AstVarRef* const origRefp = new AstVarRef{flp, vscp, VAccess::READ};
        markNonReplaceable(origRefp);
        addForceReadFinal(stmtsp, rdVscp, negVscp, origRefp, isRanged, flp);

        return stmtsp;
    }

private:
    // NODE STATE
    //  AstVarRef::user1      -> Flag indicating not to replace reference
    const VNUser1InUse m_user1InUse;
    std::unordered_map<AstVar*, ForceVarInfo>
        m_forceInfo;  // Map vars to per-scope force statements
    std::unordered_map<AstVarScope*, ForceReadInfo>
        m_forceReadInfo;  // Map varscopes to read result/negation variables
    std::unordered_map<AstVarScope*, AstVarScope*>
        m_rdToOrigVscp;  // Map __VforceRd VarScope to original VarScope

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

    const ForceReadInfo* getForceReadInfo(AstVarScope* vscp) const {
        auto it = m_forceReadInfo.find(vscp);
        if (it == m_forceReadInfo.end()) return nullptr;
        return &it->second;
    }

    // Get original VarScope from __VforceRd VarScope (for chained force handling)
    AstVarScope* getOrigVscpFromRd(AstVarScope* rdVscp) const {
        auto it = m_rdToOrigVscp.find(rdVscp);
        if (it == m_rdToOrigVscp.end()) return nullptr;
        return it->second;
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
                for (auto& forceEntry : scopeEntry.second)
                    forceEntry.second.m_enArrayVscp = enVscp;
                AstVarScope* const loopVscp = getOrCreateLoopVar(loopVarScopes, scopep, flp, varp);

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

    // Update stored RHS expressions to replace reads of forced variables with __VforceRd
    void updateRhsExpressionsForChainedForces() {
        for (auto& varEntry : m_forceInfo) {
            for (auto& scopeEntry : varEntry.second.m_scopeForces) {
                for (auto& forceEntry : scopeEntry.second) {
                    ForceInfo& finfo = forceEntry.second;
                    if (!finfo.m_rhsExprp) continue;

                    std::vector<std::pair<AstVarRef*, AstVarScope*>> refsToReplace;
                    finfo.m_rhsExprp->foreach([&](AstVarRef* refp) {
                        if (!refp->access().isReadOnly()) return;
                        if (isNotReplaceable(refp)) return;
                        AstVarScope* const refVscp = refp->varScopep();
                        if (!refVscp) return;
                        const ForceReadInfo* readInfo = getForceReadInfo(refVscp);
                        if (readInfo && readInfo->m_rdVscp) {
                            refsToReplace.push_back({refp, readInfo->m_rdVscp});
                        }
                    });

                    for (auto& [refp, rdVscp] : refsToReplace) {
                        AstVarRef* const rdRefp
                            = new AstVarRef{refp->fileline(), rdVscp, VAccess::READ};
                        if (refp == finfo.m_rhsExprp) {
                            finfo.m_rhsExprp = rdRefp;
                            VL_DO_DANGLING(refp->deleteTree(), refp);
                        } else if (refp->backp()) {
                            refp->replaceWith(rdRefp);
                            VL_DO_DANGLING(refp->deleteTree(), refp);
                        } else {
                            VL_DO_DANGLING(refp->deleteTree(), refp);
                        }
                    }
                }
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

                    bool hasVarRefs = false;
                    finfo.m_rhsExprp->foreach([&](AstVarRef*) { hasVarRefs = true; });

                    // Initial assignment
                    AstActive* const initActivep = new AstActive{
                        flp, "force-rhs-init",
                        new AstSenTree{flp, new AstSenItem{flp, AstSenItem::Static{}}}};
                    initActivep->senTreeStorep(initActivep->sentreep());
                    initActivep->addStmtsp(new AstInitial{
                        flp, new AstAssign{
                                 flp,
                                 new AstArraySel{flp, new AstVarRef{flp, rhsVscp, VAccess::WRITE},
                                                 static_cast<int>(finfo.m_forceId)},
                                 finfo.m_rhsExprp->cloneTreePure(false)}});
                    scopep->addBlocksp(initActivep);

                    // Always @(*) block if RHS has variable refs
                    if (hasVarRefs) {
                        AstActive* const activep = new AstActive{
                            flp, "force-rhs",
                            new AstSenTree{flp, new AstSenItem{flp, AstSenItem::Combo{}}}};
                        activep->senTreeStorep(activep->sentreep());
                        activep->addStmtsp(new AstAlways{
                            flp, VAlwaysKwd::ALWAYS, nullptr,
                            new AstAssign{
                                flp,
                                new AstArraySel{flp, new AstVarRef{flp, rhsVscp, VAccess::WRITE},
                                                static_cast<int>(finfo.m_forceId)},
                                finfo.m_rhsExprp->cloneTreePure(false)}});
                        scopep->addBlocksp(activep);
                    }
                }
            }
        }
    }

    // Create __VforceRd and __VforceNeg variables
    void createReadVars(std::unordered_map<AstScope*, AstVarScope*>& loopVarScopes) {
        for (auto& varEntry : m_forceInfo) {
            AstVar* const varp = varEntry.first;
            auto& varInfo = varEntry.second;
            if (varInfo.m_scopeForces.empty()) continue;
            const size_t numForces = varInfo.m_maxForces;
            if (!numForces) continue;

            FileLine* const flp = varp->fileline();
            AstNodeDType* const dtypep = varp->dtypep();
            AstNodeDType* const enDTypep
                = ForceState::isRangedDType(varp) ? varp->dtypep() : varp->findBitDType();

            AstVar* const rdVarp
                = new AstVar{flp, VVarType::VAR, varp->name() + "__VforceRd", dtypep};
            rdVarp->isInternal(true);
            rdVarp->trace(false);
            varp->addNextHere(rdVarp);

            AstVar* const negVarp
                = new AstVar{flp, VVarType::VAR, varp->name() + "__VforceNeg", enDTypep};
            negVarp->isInternal(true);
            negVarp->trace(false);
            varp->addNextHere(negVarp);

            for (auto& scopeEntry : varInfo.m_scopeForces) {
                AstVarScope* const origVscp = scopeEntry.first;
                AstScope* const scopep = origVscp->scopep();
                AstVarScope* const rdVscp = new AstVarScope{flp, scopep, rdVarp};
                scopep->addVarsp(rdVscp);
                AstVarScope* const negVscp = new AstVarScope{flp, scopep, negVarp};
                scopep->addVarsp(negVscp);
                m_forceReadInfo[origVscp] = ForceReadInfo{rdVscp, negVscp};
                m_rdToOrigVscp[rdVscp] = origVscp;  // Reverse mapping
                getOrCreateLoopVar(loopVarScopes, scopep, flp, varp);
            }
        }
    }

    // Create always @(*) blocks that compute the force read value using a loop
    void createReadAlwaysBlocks(std::unordered_map<AstScope*, AstVarScope*>& loopVarScopes) {
        for (auto& varEntry : m_forceInfo) {
            AstVar* const varp = varEntry.first;
            auto& varInfo = varEntry.second;
            if (varInfo.m_scopeForces.empty()) continue;
            const size_t numForces = varInfo.m_maxForces;
            if (!numForces) continue;

            FileLine* const flp = varp->fileline();
            const bool isRanged = isRangedDType(varp);

            for (auto& scopeEntry : varInfo.m_scopeForces) {
                AstVarScope* const origVscp = scopeEntry.first;
                AstScope* const scopep = origVscp->scopep();

                const ForceReadInfo* readInfo = getForceReadInfo(origVscp);
                UASSERT_OBJ(readInfo, origVscp, "ForceReadInfo not found");
                AstVarScope* const rdVscp = readInfo->m_rdVscp;
                AstVarScope* const negVscp = readInfo->m_negVscp;
                AstVarScope* const loopVscp = loopVarScopes[scopep];
                UASSERT_OBJ(loopVscp, scopep, "Loop variable not found");

                AstVarScope* const enArrayVscp = scopeEntry.second.begin()->second.m_enArrayVscp;
                AstVarScope* const rhsArrayVscp = scopeEntry.second.begin()->second.m_rhsArrayVscp;
                UASSERT_OBJ(rhsArrayVscp, origVscp, "RHS array VarScope not set");

                // Init: __VforceRd = 0; __VforceNeg = 0; __Vforcei = 0;
                AstNode* loopStmtsp
                    = new AstAssign{flp, new AstVarRef{flp, rdVscp, VAccess::WRITE},
                                    makeZeroConst(varp, varp->dtypep()->width())};
                loopStmtsp->addNext(
                    new AstAssign{flp, new AstVarRef{flp, negVscp, VAccess::WRITE},
                                  makeZeroConst(varp, isRanged ? varp->dtypep()->width() : 1)});
                loopStmtsp->addNext(new AstAssign{
                    flp, new AstVarRef{flp, loopVscp, VAccess::WRITE}, new AstConst{flp, 0}});

                AstLoop* const loopp = new AstLoop{flp};
                loopStmtsp->addNext(loopp);

                loopp->addStmtsp(new AstLoopTest{
                    flp, loopp,
                    new AstLt{flp, new AstVarRef{flp, loopVscp, VAccess::READ},
                              new AstConst{flp, static_cast<uint32_t>(numForces)}}});

                addForceReadLoopBody(loopp, rdVscp, negVscp, enArrayVscp, rhsArrayVscp, loopVscp,
                                     isRanged, flp);

                loopp->addStmtsp(
                    new AstAssign{flp, new AstVarRef{flp, loopVscp, VAccess::WRITE},
                                  new AstAdd{flp, new AstVarRef{flp, loopVscp, VAccess::READ},
                                             new AstConst{flp, 1}}});

                AstVarRef* const origRefp = new AstVarRef{flp, origVscp, VAccess::READ};
                markNonReplaceable(origRefp);
                addForceReadFinal(loopStmtsp, rdVscp, negVscp, origRefp, isRanged, flp);

                AstSenItem* const senItemp = new AstSenItem{flp, AstSenItem::Combo{}};
                AstActive* const activep
                    = new AstActive{flp, "force-read", new AstSenTree{flp, senItemp}};
                activep->senTreeStorep(activep->sentreep());
                activep->addStmtsp(new AstAlways{flp, VAlwaysKwd::ALWAYS, nullptr, loopStmtsp});
                scopep->addBlocksp(activep);
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

    // VISITORS
    void visit(AstAssignForce* nodep) override {
        UINFO(2, "Converting force statement: " << nodep);

        FileLine* flp = nodep->fileline();
        AstNodeExpr* const lhsp = nodep->lhsp();
        AstVarRef* const lhsVarRefp = m_state.getOneVarRef(lhsp);
        AstVar* const forcedVarp = lhsVarRefp->varp();
        AstVarScope* const forceVscp = lhsVarRefp->varScopep();
        UASSERT_OBJ(forcedVarp, lhsp, "VarRef missing Varp");
        UASSERT_OBJ(forceVscp, lhsp, "VarRef missing VarScope");

        V3Number ones{lhsp, ForceState::isRangedDType(lhsp) ? lhsp->width() : 1};
        ones.setAllBits1();
        AstNodeExpr* currentEnAllOnesp = new AstConst{flp, ones};
        ForceState::ForceInfo currentForceInfo = m_state.getForceInfo(nodep);

        // Set current __VforceEns[ID] to all ones
        AstAssign* const enableCurrentp = new AstAssign{
            flp, ForceState::createEnableExpr(currentForceInfo, lhsp), currentEnAllOnesp};
        AstNode* stmtListp = enableCurrentp;

        // For chained forces: if the RHS references __VforceRd variables, we need to compute
        // their values first, because the always @(*) blocks may not have run yet
        if (currentForceInfo.m_rhsExprp) {
            // Find all __VforceRd VarScope references in the RHS expression
            std::set<AstVarScope*> rdVscpsNeeded;
            currentForceInfo.m_rhsExprp->foreach([&](AstVarRef* refp) {
                if (!refp->access().isReadOnly()) return;
                AstVarScope* const refVscp = refp->varScopep();
                if (!refVscp) return;
                AstVarScope* const origVscp = m_state.getOrigVscpFromRd(refVscp);
                if (origVscp) { rdVscpsNeeded.insert(origVscp); }
            });

            // Generate force read loops for each dependent forced variable
            for (AstVarScope* origVscp : rdVscpsNeeded) {
                const ForceState::ForceReadInfo* readInfo = m_state.getForceReadInfo(origVscp);
                UASSERT_OBJ(readInfo, origVscp, "ForceReadInfo not found");
                AstVar* const depVarp = origVscp->varp();
                const auto& depForces = m_state.getForceInfos(origVscp);
                if (depForces.empty()) continue;
                const size_t depNumForces = m_state.getNumForces(depVarp);
                if (!depNumForces) continue;

                AstNode* depLoop = ForceState::createForceReadLoop(
                    origVscp, readInfo->m_rdVscp, readInfo->m_negVscp, depForces, depNumForces,
                    m_loopVarScopes, flp);
                stmtListp->addNext(depLoop);
            }
        }

        // Update __VforceRHS[ID] with the current RHS value for immediate consistency
        // This is needed because the always @(*) blocks may not have run yet within the same
        // timestep
        if (currentForceInfo.m_rhsExprp && currentForceInfo.m_rhsArrayVscp) {
            AstAssign* const rhsUpdatep = new AstAssign{
                flp,
                new AstArraySel{
                    flp, new AstVarRef{flp, currentForceInfo.m_rhsArrayVscp, VAccess::WRITE},
                    static_cast<int>(currentForceInfo.m_forceId)},
                currentForceInfo.m_rhsExprp->cloneTreePure(false)};
            stmtListp->addNext(rhsUpdatep);
        }

        // AND other __VforceEns[ID] with negation of current range using a loop
        const size_t numForces = m_state.getNumForces(forcedVarp);
        if (numForces > 1) {
            AstScope* const scopep = forceVscp->scopep();
            AstVarScope* const loopVscp
                = ForceState::getOrCreateLoopVar(m_loopVarScopes, scopep, flp, forcedVarp);

            AstNode* loopStmtsp = new AstAssign{flp, new AstVarRef{flp, loopVscp, VAccess::WRITE},
                                                new AstConst{flp, 0}};

            AstLoop* const loopp = new AstLoop{flp};
            loopStmtsp->addNext(loopp);

            loopp->addStmtsp(
                new AstLoopTest{flp, loopp,
                                new AstLt{flp, new AstVarRef{flp, loopVscp, VAccess::READ},
                                          new AstConst{flp, static_cast<uint32_t>(numForces)}}});

            AstNodeExpr* const lhsForLoop = ForceState::createEnableExprWithVarIndex(
                currentForceInfo.m_enArrayVscp, lhsp, loopVscp, VAccess::WRITE);
            AstNodeExpr* const rhsForLoopRead = ForceState::createEnableExprWithVarIndex(
                currentForceInfo.m_enArrayVscp, lhsp, loopVscp, VAccess::READ);
            AstAssign* const disableOtherp = new AstAssign{
                flp, lhsForLoop,
                new AstAnd{flp, rhsForLoopRead,
                           new AstNot{flp, currentEnAllOnesp->cloneTreePure(false)}}};

            loopp->addStmtsp(new AstIf{
                flp,
                new AstNeq{flp, new AstVarRef{flp, loopVscp, VAccess::READ},
                           new AstConst{flp, static_cast<uint32_t>(currentForceInfo.m_forceId)}},
                disableOtherp, nullptr});

            loopp->addStmtsp(
                new AstAssign{flp, new AstVarRef{flp, loopVscp, VAccess::WRITE},
                              new AstAdd{flp, new AstVarRef{flp, loopVscp, VAccess::READ},
                                         new AstConst{flp, 1}}});

            stmtListp->addNext(loopStmtsp);
        }

        UINFO(3, "Replaced force with enable logic: " << forcedVarp->name());
        UASSERT_OBJ(stmtListp, nodep, "Empty statement list");
        nodep->replaceWith(stmtListp);
        VL_DO_DANGLING(pushDeletep(nodep), nodep);
    }

    void visit(AstRelease* nodep) override {
        UINFO(2, "Converting release statement: " << nodep);
        AstNodeExpr* const lhsp = nodep->lhsp();
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
        const bool isRanged = ForceState::isRangedDType(releasedVarp);
        AstNode* stmtListp = nullptr;

        // For non-continuous vars, save the forced value before disabling using loop
        const ForceState::ForceReadInfo* readInfo = m_state.getForceReadInfo(releaseVscp);
        if (!releasedVarp->isContinuously() && readInfo && readInfo->m_rdVscp) {
            // For chained forces: update RHS from dependent __VforceRd variables
            // This is needed because the always @(*) blocks may not have run yet
            for (const auto& force : forces) {
                const ForceState::ForceInfo& finfo = force.second;
                if (!finfo.m_rhsExprp) continue;

                // Find __VforceRd VarScope references in the RHS expression
                finfo.m_rhsExprp->foreach([&](AstVarRef* refp) {
                    if (!refp->access().isReadOnly()) return;
                    AstVarScope* const refVscp = refp->varScopep();
                    if (!refVscp) return;
                    AstVarScope* const origVscp = m_state.getOrigVscpFromRd(refVscp);
                    if (!origVscp) return;

                    // Update the RHS entry with the current __VforceRd value
                    AstAssign* const rhsUpdatep = new AstAssign{
                        flp,
                        new AstArraySel{flp,
                                        new AstVarRef{flp, finfo.m_rhsArrayVscp, VAccess::WRITE},
                                        static_cast<int>(finfo.m_forceId)},
                        finfo.m_rhsExprp->cloneTreePure(false)};
                    if (!stmtListp) {
                        stmtListp = rhsUpdatep;
                    } else {
                        stmtListp->addNext(rhsUpdatep);
                    }
                });
            }

            // Use the loop to compute the current forced value
            AstNode* const readLoop = ForceState::createForceReadLoop(
                releaseVscp, readInfo->m_rdVscp, readInfo->m_negVscp, forces,
                m_state.getNumForces(releasedVarp), m_loopVarScopes, flp);
            if (!stmtListp) {
                stmtListp = readLoop;
            } else {
                stmtListp->addNext(readLoop);
            }
            // Assign the computed value to the variable
            stmtListp->addNext(
                new AstAssign{flp, lhsp->cloneTreePure(false),
                              new AstVarRef{flp, readInfo->m_rdVscp, VAccess::READ}});
        }

        // Clear all force enables using a loop
        const size_t numForces = m_state.getNumForces(releasedVarp);
        if (numForces > 0) {
            AstScope* const scopep = releaseVscp->scopep();
            AstVarScope* const loopVscp
                = ForceState::getOrCreateLoopVar(m_loopVarScopes, scopep, flp, releasedVarp);
            AstVarScope* const enArrayVscp = forces.begin()->second.m_enArrayVscp;

            // Init: __Vforcei = 0
            AstNode* loopStmtsp = new AstAssign{flp, new AstVarRef{flp, loopVscp, VAccess::WRITE},
                                                new AstConst{flp, 0}};

            AstLoop* const loopp = new AstLoop{flp};
            loopStmtsp->addNext(loopp);

            // __Vforcei < numForces
            loopp->addStmtsp(
                new AstLoopTest{flp, loopp,
                                new AstLt{flp, new AstVarRef{flp, loopVscp, VAccess::READ},
                                          new AstConst{flp, static_cast<uint32_t>(numForces)}}});

            // __VforceEns[__Vforcei] = 0
            AstNodeExpr* const enLhsp = ForceState::createEnableExprWithVarIndex(
                enArrayVscp, lhsp, loopVscp, VAccess::WRITE);
            loopp->addStmtsp(new AstAssign{
                flp, enLhsp, ForceState::makeZeroConst(lhsp, isRanged ? lhsp->width() : 1)});

            // __Vforcei++
            loopp->addStmtsp(
                new AstAssign{flp, new AstVarRef{flp, loopVscp, VAccess::WRITE},
                              new AstAdd{flp, new AstVarRef{flp, loopVscp, VAccess::READ},
                                         new AstConst{flp, 1}}});

            if (!stmtListp) {
                stmtListp = loopStmtsp;
            } else {
                stmtListp->addNext(loopStmtsp);
            }
        }

        // Update __VforceRd for chained forces (after clearing enables)
        if (readInfo && readInfo->m_rdVscp) {
            AstVarRef* const origRefp = new AstVarRef{flp, releaseVscp, VAccess::READ};
            ForceState::markNonReplaceable(origRefp);
            stmtListp->addNext(new AstAssign{
                flp, new AstVarRef{flp, readInfo->m_rdVscp, VAccess::WRITE}, origRefp});
        }

        nodep->replaceWith(stmtListp);
        VL_DO_DANGLING(pushDeletep(nodep), nodep);
        UINFO(3, "Replaced release with disable logic");
    }

    void visit(AstNode* nodep) override { iterateChildren(nodep); }

public:
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
    std::unordered_map<AstScope*, AstVarScope*>& m_loopVarScopes;

    void visit(AstVarRef* nodep) override {
        if (ForceState::isNotReplaceable(nodep)) return;

        AstVarScope* const vscp = nodep->varScopep();
        const std::unordered_map<AstAssignForce*, ForceState::ForceInfo>& forces
            = m_state.getForceInfos(vscp);
        if (forces.empty()) return;

        if (nodep->access().isRW()) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: Signals used via read-write reference cannot be forced");
            return;
        }
        if (!nodep->access().isReadOnly()) return;

        const ForceState::ForceReadInfo* const readInfo = m_state.getForceReadInfo(vscp);
        UASSERT_OBJ(readInfo && readInfo->m_rdVscp, nodep,
                    "ForceReadInfo not found for forced variable");

        FileLine* const flp = nodep->fileline();

        // Find enclosing procedural block
        AstNode* stmtp = nodep;
        bool inProceduralContext = false;
        while (stmtp) {
            if (VN_IS(stmtp, Initial) || VN_IS(stmtp, Always) || VN_IS(stmtp, AlwaysPost)
                || VN_IS(stmtp, AlwaysPre)) {
                inProceduralContext = true;
                break;
            }
            stmtp = stmtp->backp();
        }

        if (inProceduralContext) {
            // Insert loop before statement for fresh value
            stmtp = nodep;
            while (stmtp && !VN_IS(stmtp, NodeStmt)) stmtp = stmtp->backp();
            if (stmtp) {
                stmtp->addHereThisAsNext(ForceState::createForceReadLoop(
                    vscp, readInfo->m_rdVscp, readInfo->m_negVscp, forces,
                    m_state.getNumForces(vscp->varp()), m_loopVarScopes, flp));
            }
        }

        nodep->replaceWith(new AstVarRef{flp, readInfo->m_rdVscp, VAccess::READ});
        VL_DO_DANGLING(pushDeletep(nodep), nodep);
    }

    void visit(AstNode* nodep) override { iterateChildren(nodep); }

public:
    ForceReplaceVisitor(AstNetlist* nodep, const ForceState& state,
                        std::unordered_map<AstScope*, AstVarScope*>& loopVarScopes)
        : m_state{state}
        , m_loopVarScopes{loopVarScopes} {
        iterateAndNextNull(nodep->modulesp());
    }
};

void V3Force::forceAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    if (!v3Global.hasForceableSignals()) return;
    ForceState state;
    std::unordered_map<AstScope*, AstVarScope*> loopVarScopes;
    { ForceDiscoveryVisitor{nodep, state}; }
    state.createEnStorage(loopVarScopes);
    state.createReadVars(loopVarScopes);
    state.updateRhsExpressionsForChainedForces();
    state.createRhsStorage();
    state.createReadAlwaysBlocks(loopVarScopes);
    { ForceConvertVisitor{nodep, state, loopVarScopes}; }
    { ForceReplaceVisitor{nodep, state, loopVarScopes}; }
    state.cleanupRhsExpressions();
    V3Global::dumpCheckGlobalTree("force", 0, dumpTreeEitherLevel() >= 3);
}
