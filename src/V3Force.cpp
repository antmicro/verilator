// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Convert forceable signals, process force/release
//
// Code available from: https://verilator.org
//
//*************************************************************************
//
// Copyright 2003-2026 by Wilson Snyder. This program is free software; you
// can redistribute it and/or modify it under the terms of either the GNU
// Lesser General Public License Version 3 or the Perl Artistic License
// Version 2.0.
// SPDX-License-Identifier: LGPL-3.0-only OR Artistic-2.0
//
//*************************************************************************
//  V3Force's Transformations:
//
//  For each forceable var/net "<name>":
//    - Create <name>__VforceVec (VlForceVec) to track active force ranges
//    - Create <name>__VforceRhs (array) to hold RHS shadow values
//    - Add continuous assignments: <name>__VforceRhs[ID] = RHS
//
//  For each `force <name>[range] = <RHS>` with ID:
//    - <name>__VforceVec.addForce(lsb, msb, ID)
//
//  For each `release <name>[range]`:
//    - If not continuously driven: <name> = VlForceRead(<name>, __VforceVec, __VforceRhs)
//    - <name>__VforceVec.release(lsb, msb)
//
//  For each read of <name>:
//    - Replace with: VlForceRead(<name>, __VforceVec, __VforceRhs)
//
//*************************************************************************

#include "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#include "V3Force.h"

#include "V3AstUserAllocator.h"

VL_DEFINE_DEBUG_FUNCTIONS;

class ForceState final {
public:
    struct ForceInfo final {
        int m_forceId;
        int m_rangeLsb;  // VlForceVec range: bit index or array element index
        int m_rangeMsb;
        int m_padLsb;  // Bit positions for RHS padding
        int m_padMsb;
        AstNodeExpr* m_rhsExprp;
        std::vector<int> m_arrayIndices;
    };

    struct VarForceInfo final {
        AstVarScope* m_forceVecVscp = nullptr;
        AstVarScope* m_rhsArrayVscp = nullptr;
        AstVarScope* m_forceEnVscp = nullptr;
        AstVarScope* m_forceValVscp = nullptr;
        AstScope* m_scopep = nullptr;
        std::unordered_map<AstAssignForce*, ForceInfo> m_forces;
    };

    struct ArraySelInfo final {
        std::vector<AstArraySel*> m_sels;
        bool m_hasBitSel = false;
    };

    struct ForceRangeInfo final {
        int m_padLsb = 0;
        int m_padMsb = 0;
        int m_rangeLsb = 0;
        int m_rangeMsb = 0;
        bool m_hasArraySel = false;
        ArraySelInfo m_arrayInfo;
        std::vector<int> m_arrayIndices;
    };

private:
    const VNUser1InUse m_user1InUse;
    const VNUser2InUse m_user2InUse;

    std::unordered_map<AstVar*, VarForceInfo> m_varInfo;

public:
    ForceState() = default;
    VL_UNCOPYABLE(ForceState);

    void addExternalForce(AstVarScope* scopep, AstVar* varp) {
        FileLine* const flp = varp->fileline();
        AstNodeDType* const enDtypep
            = isBitwiseDType(varp) ? varp->dtypep() : varp->findBitDType();
        AstVar* const enVarp
            = new AstVar{flp, VVarType::VAR, varp->name() + "__VforceEn", enDtypep};
        enVarp->sigUserRWPublic(true);
        AstVar* const valVarp
            = new AstVar{flp, VVarType::VAR, varp->name() + "__VforceVal", varp->dtypep()};
        valVarp->sigUserRWPublic(true);
        varp->addNextHere(enVarp);
        varp->addNextHere(valVarp);
        AstVarScope* const enVscp = new AstVarScope{flp, scopep->scopep(), enVarp};
        AstVarScope* const valVscp = new AstVarScope{flp, scopep->scopep(), valVarp};
        scopep->scopep()->addVarsp(enVscp);
        scopep->scopep()->addVarsp(valVscp);
        VarForceInfo& info = getOrCreateVarInfo(varp);
        info.m_forceEnVscp = enVscp;
        info.m_forceValVscp = valVscp;
        AstSenItem* const itemsp = new AstSenItem{flp, VEdgeType::ET_CHANGED,
                                                  new AstVarRef{flp, enVscp, VAccess::READ}};
        AstActive* const activep = new AstActive{flp, "force-update", new AstSenTree{flp, itemsp}};
        activep->senTreeStorep(activep->sentreep());
        AstVarRef* const refp = new AstVarRef{flp, scopep, VAccess::READ};
        markNonReplaceable(refp);
        AstVarRef* const enRefp = new AstVarRef{flp, enVscp, VAccess::READ};
        AstVarRef* const valRefp = new AstVarRef{flp, valVscp, VAccess::READ};
        AstNodeExpr* const forceExprp
            = isRangedDType(varp)
                  ? static_cast<AstNodeExpr*>(new AstOr{
                        flp, new AstAnd{flp, enRefp, valRefp},
                        new AstAnd{flp, new AstNot{flp, enRefp->cloneTreePure(false)}, refp}})
                  : static_cast<AstNodeExpr*>(new AstCond{flp, enRefp, valRefp, refp});
        AstAssignForce* const forceAssignp
            = new AstAssignForce{flp, new AstVarRef{flp, scopep, VAccess::WRITE}, forceExprp};
        forceAssignp->user2(true);
        activep->addStmtsp(new AstAlways{flp, VAlwaysKwd::ALWAYS, nullptr, forceAssignp});
        scopep->scopep()->addBlocksp(activep);

        AstNodeExpr* const rhsClonep = forceExprp->cloneTreePure(false);
        rhsClonep->foreach([varp](AstVarRef* const refp) {
            if (refp->varp() == varp) ForceState::markNonReplaceable(refp);
        });
        const int padMsb = isBitwiseDType(varp) && varp->width() ? (varp->width() - 1) : 0;
        int rangeLsb = 0;
        int rangeMsb = padMsb;
        if (isUnpackedArrayDType(varp->dtypep())) {
            fullUnpackedRange(varp->dtypep(), rangeLsb, rangeMsb);
        }
        addForceAssignment(varp, scopep, rhsClonep, forceAssignp, rangeLsb, rangeMsb, 0, padMsb,
                           {});
    }

    // STATIC METHODS
    static AstConst* makeZeroConst(AstNode* nodep, int width) {
        V3Number zero{nodep, width};
        zero.setAllBits0();
        return new AstConst{nodep->fileline(), zero};
    }

    static AstConst* makeConst32(FileLine* flp, int value) {
        return new AstConst{flp, AstConst::WidthedValue{}, 32, static_cast<uint32_t>(value)};
    }

    static AstConst* makeRangeMaskConst(AstNode* nodep, int width, int lsb, int msb) {
        V3Number mask{nodep, width};
        mask.setAllBits0();
        if (width > 0 && msb >= lsb) {
            for (int bit = lsb; bit <= msb; ++bit) mask.setBit(bit, 1);
        }
        return new AstConst{nodep->fileline(), mask};
    }

    static AstNodeExpr* padRhsToVarWidth(AstNodeExpr* rhsExprp, AstVar* varp, int padLsb,
                                         int padMsb) {
        if (!isBitwiseDType(varp)) return rhsExprp;
        const int varWidth = varp->width();
        if (varWidth <= 0) return rhsExprp;
        const int lowPad = padLsb;
        const int highPad = varWidth - (padMsb + 1);
        if (lowPad > 0) {
            rhsExprp
                = new AstConcat{rhsExprp->fileline(), rhsExprp, makeZeroConst(rhsExprp, lowPad)};
        }
        if (highPad > 0) {
            rhsExprp
                = new AstConcat{rhsExprp->fileline(), makeZeroConst(rhsExprp, highPad), rhsExprp};
        }
        return rhsExprp;
    }

    static bool isUnpackedArrayDType(const AstNodeDType* dtypep) {
        return VN_IS(dtypep->skipRefp(), UnpackArrayDType);
    }

    static int totalUnpackedElements(const AstNodeDType* dtypep) {
        const AstNodeDType* curp = dtypep->skipRefp();
        if (!VN_IS(curp, UnpackArrayDType)) return 0;
        int total = 1;
        while (const AstUnpackArrayDType* const dimp = VN_CAST(curp, UnpackArrayDType)) {
            total *= dimp->declRange().elements();
            curp = dimp->subDTypep() ? dimp->subDTypep()->skipRefp() : nullptr;
        }
        return total;
    }

    static void fullUnpackedRange(const AstNodeDType* dtypep, int& lsb, int& msb) {
        const int total = totalUnpackedElements(dtypep);
        lsb = 0;
        msb = total > 0 ? (total - 1) : 0;
    }

    static bool isRangedDType(AstNode* nodep) {
        const AstBasicDType* const basicp = nodep->dtypep()->skipRefp()->basicp();
        return basicp && basicp->isRanged();
    }

    static bool isBitwiseDType(AstNode* nodep) {
        const AstBasicDType* const basicp = nodep->dtypep()->skipRefp()->basicp();
        return basicp && !basicp->isDouble() && !basicp->isString() && !basicp->isOpaque();
    }

    static AstNodeExpr* castToVarDType(AstNodeExpr* exprp, AstVar* varp) {
        const AstNodeDType* const dtypep = varp->dtypep()->skipRefp();
        const AstBasicDType* const basicp = dtypep->basicp();
        if (!basicp || basicp->isDouble() || basicp->isString() || basicp->isOpaque()
            || dtypep->isWide() || isUnpackedArrayDType(dtypep)) {
            return exprp;
        }
        return new AstCCast{exprp->fileline(), exprp, varp->dtypep()};
    }

    static bool isNotReplaceable(const AstVarRef* const nodep) { return nodep->user1(); }
    static void markNonReplaceable(AstVarRef* const nodep) { nodep->user1SetOnce(); }

    static AstVarRef* getOneVarRef(AstNodeExpr* forceStmtp) {
        AstNode* const basep = AstArraySel::baseFromp(forceStmtp, true);
        AstVarRef* const varRefp = VN_CAST(basep, VarRef);
        UASSERT_OBJ(varRefp, forceStmtp, "`force` assignment has no VarRef on LHS");
        return varRefp;
    }

    static ForceRangeInfo getForceRangeInfo(AstNodeExpr* lhsp, AstVar* varp,
                                            bool requireConstRangeSelect, bool needArrayIndices,
                                            const char* bitSelMsg) {
        ForceRangeInfo info;
        info.m_arrayInfo = getArraySelInfo(lhsp);
        info.m_hasArraySel = !info.m_arrayInfo.m_sels.empty();

        info.m_padMsb = isBitwiseDType(varp) ? (varp->width() - 1) : 0;

        if (const AstSel* const selp = VN_CAST(lhsp, Sel)) {
            if (requireConstRangeSelect) {
                UASSERT_OBJ(VN_IS(selp->lsbp(), Const), lhsp,
                            "Unsupported: force on non-const range select");
            }
            info.m_padLsb = selp->lsbConst();
            info.m_padMsb = info.m_padLsb + selp->widthConst() - 1;
        }

        UASSERT_OBJ(!(info.m_hasArraySel && info.m_arrayInfo.m_hasBitSel), lhsp, bitSelMsg);

        info.m_rangeLsb = info.m_padLsb;
        info.m_rangeMsb = info.m_padMsb;
        if (info.m_hasArraySel) {
            std::vector<int> indices;
            indices.reserve(info.m_arrayInfo.m_sels.size());
            for (AstArraySel* const selp : info.m_arrayInfo.m_sels) {
                UASSERT_OBJ(VN_IS(selp->bitp(), Const), selp,
                            "Unsupported: force on non-constant array select");
                indices.push_back(VN_AS(selp->bitp(), Const)->toSInt());
            }
            info.m_rangeLsb = flattenIndex(indices, arraySelDimSizes(info.m_arrayInfo));
            if (needArrayIndices) info.m_arrayIndices = std::move(indices);

            info.m_rangeMsb = info.m_rangeLsb;
        } else if (isUnpackedArrayDType(varp->dtypep())) {
            fullUnpackedRange(varp->dtypep(), info.m_rangeLsb, info.m_rangeMsb);
        }
        return info;
    }

    AstNodeExpr* createForceReadCall(const VarForceInfo& varInfo, FileLine* flp,
                                     const string& funcName, const string& cType,
                                     AstNodeExpr* origValp, AstNode* dtypeFromp,
                                     AstNodeExpr* indexExprp) const {
        UASSERT(varInfo.m_forceVecVscp, "No forceVec for forced variable");
        UASSERT(varInfo.m_rhsArrayVscp, "No RHS array for forced variable");

        AstCExpr* const callp = new AstCExpr{flp, AstCExpr::Pure{}};
        callp->add(funcName + "<" + cType + ">(");
        callp->add(origValp);
        callp->add(", ");
        callp->add(new AstVarRef{flp, varInfo.m_forceVecVscp, VAccess::READ});
        callp->add(", &(");
        callp->add(
            new AstArraySel{flp, new AstVarRef{flp, varInfo.m_rhsArrayVscp, VAccess::READ}, 0});
        callp->add(")");
        if (indexExprp) {
            callp->add(", ");
            callp->add(indexExprp);
        }
        callp->add(")");
        callp->dtypeFrom(dtypeFromp);
        return callp;
    }

    VarForceInfo& getOrCreateVarInfo(AstVar* varp) { return m_varInfo[varp]; }

    const VarForceInfo* getVarInfo(AstVar* varp) const {
        auto it = m_varInfo.find(varp);
        return it != m_varInfo.end() ? &it->second : nullptr;
    }

    void addForceAssignment(AstVar* varp, AstVarScope* vscp, AstNodeExpr* rhsExprp,
                            AstAssignForce* forceStmtp, int rangeLsb, int rangeMsb, int padLsb,
                            int padMsb, std::vector<int> arrayIndices) {
        v3Global.setUsesForce();
        varp->setForcedByCode();

        VarForceInfo& info = getOrCreateVarInfo(varp);
        if (!info.m_scopep) info.m_scopep = vscp->scopep();
        const int forceId = info.m_forces.size();
        FileLine* const flp = varp->fileline();
        AstScope* const scopep = vscp->scopep();
        if (!info.m_forceVecVscp) {
            AstCDType* const forceVecDtypep = new AstCDType{flp, "VlForceVec"};
            v3Global.rootp()->typeTablep()->addTypesp(forceVecDtypep);

            AstVar* const forceVecVarp
                = new AstVar{flp, VVarType::MEMBER, varp->name() + "__VforceVec", forceVecDtypep};
            forceVecVarp->funcLocal(false);
            forceVecVarp->isInternal(true);
            varp->addNextHere(forceVecVarp);
            info.m_forceVecVscp = new AstVarScope{flp, scopep, forceVecVarp};
            scopep->addVarsp(info.m_forceVecVscp);
        }

        info.m_forces.emplace(forceStmtp, ForceInfo{forceId, rangeLsb, rangeMsb, padLsb, padMsb,
                                                    rhsExprp, std::move(arrayIndices)});

        UINFO(3, "Added force ID " << forceId << " for " << varp->name() << " [" << rangeMsb << ":"
                                   << rangeLsb << "]\n");
    }

    static void collectArraySelInfo(AstNodeExpr* exprp, ArraySelInfo& info) {
        if (!exprp) return;
        if (const auto* const selp = VN_CAST(exprp, Sel)) {
            info.m_hasBitSel = true;
            collectArraySelInfo(selp->fromp(), info);
        } else if (auto* const selp = VN_CAST(exprp, ArraySel)) {
            collectArraySelInfo(selp->fromp(), info);
            info.m_sels.push_back(selp);
        } else if (const auto* const memberp = VN_CAST(exprp, MemberSel)) {
            collectArraySelInfo(memberp->fromp(), info);
        } else if (const auto* const memberp = VN_CAST(exprp, StructSel)) {
            collectArraySelInfo(memberp->fromp(), info);
        }
    }

    static ArraySelInfo getArraySelInfo(AstNodeExpr* exprp) {
        ArraySelInfo info;
        collectArraySelInfo(exprp, info);
        return info;
    }

    static std::vector<int> arraySelDimSizes(const ArraySelInfo& info) {
        std::vector<int> dims;
        dims.reserve(info.m_sels.size());
        for (AstArraySel* const selp : info.m_sels) {
            AstNodeDType* const dtypep = selp->fromp()->dtypep()->skipRefp();
            const AstUnpackArrayDType* const arrayp = VN_CAST(dtypep, UnpackArrayDType);
            UASSERT_OBJ(arrayp, selp, "Array select is not on unpacked array");
            dims.push_back(arrayp->declRange().elements());
        }
        return dims;
    }

    static int flattenIndex(const std::vector<int>& indices, const std::vector<int>& dimSizes) {
        UASSERT(indices.size() == dimSizes.size(), "Array index and dimension size mismatch");
        int index = 0;
        int stride = 1;
        for (int i = static_cast<int>(indices.size()) - 1; i >= 0; --i) {
            index += indices[i] * stride;
            stride *= dimSizes[i];
        }
        return index;
    }

    static AstNodeExpr* buildFlattenIndexExpr(FileLine* flp, const ArraySelInfo& info) {
        if (info.m_sels.empty()) return makeConst32(flp, 0);
        const std::vector<int> dimSizes = arraySelDimSizes(info);
        std::vector<int> constIndices;
        constIndices.reserve(info.m_sels.size());
        for (AstArraySel* const selp : info.m_sels) {
            const AstConst* const constp = VN_CAST(selp->bitp(), Const);
            if (!constp) break;
            constIndices.push_back(constp->toSInt());
        }
        if (constIndices.size() == info.m_sels.size()) {
            return makeConst32(flp, flattenIndex(constIndices, dimSizes));
        }
        AstNodeExpr* exprp = nullptr;
        int stride = 1;
        for (int i = static_cast<int>(dimSizes.size()) - 1; i >= 0; --i) {
            AstNodeExpr* termp = info.m_sels[i]->bitp()->cloneTreePure(false);
            if (stride != 1) {
                AstNodeExpr* const mulp = new AstMul{flp, termp, makeConst32(flp, stride)};
                mulp->dtypeFrom(termp);
                termp = mulp;
            }
            exprp = exprp ? new AstAdd{flp, termp, exprp} : termp;
            if (exprp != termp) exprp->dtypeFrom(termp);
            stride *= dimSizes[i];
        }
        return exprp;
    }

    static AstNodeExpr* buildRhsArraySel(FileLine* flp, AstVarScope* rhsArrayVscp, int forceId,
                                         const std::vector<int>& indices, VAccess access) {
        AstNodeExpr* exprp = new AstVarRef{flp, rhsArrayVscp, access};
        exprp = new AstArraySel{flp, exprp, forceId};
        for (const int idx : indices) exprp = new AstArraySel{flp, exprp, idx};
        return exprp;
    }

    void finalizeRhsArrays() {
        for (auto& it : m_varInfo) {
            AstVar* const varp = it.first;
            VarForceInfo& info = it.second;
            if (info.m_forces.empty() || info.m_rhsArrayVscp) continue;

            AstScope* const scopep
                = info.m_scopep ? info.m_scopep
                                : (info.m_forceVecVscp ? info.m_forceVecVscp->scopep() : nullptr);
            UASSERT_OBJ(scopep, varp, "Missing scope for force RHS array");

            FileLine* const flp = varp->fileline();
            AstRange* const rangep
                = new AstRange{flp, VNumRange{static_cast<int>(info.m_forces.size()) - 1, 0}};
            AstUnpackArrayDType* const arrayDtypep
                = new AstUnpackArrayDType{flp, varp->dtypep(), rangep};
            v3Global.rootp()->typeTablep()->addTypesp(arrayDtypep);
            AstVar* const rhsArrayVarp
                = new AstVar{flp, VVarType::VAR, varp->name() + "__VforceRhs", arrayDtypep};
            rhsArrayVarp->noSubst(true);  // Keep RHS shadow array stable for force tracking.
            varp->addNextHere(rhsArrayVarp);
            info.m_rhsArrayVscp = new AstVarScope{flp, scopep, rhsArrayVarp};
            scopep->addVarsp(info.m_rhsArrayVscp);

            for (const auto& fit : info.m_forces) {
                const ForceInfo& finfo = fit.second;
                if (!finfo.m_rhsExprp) continue;
                AstNodeExpr* const arraySelp
                    = buildRhsArraySel(flp, info.m_rhsArrayVscp, finfo.m_forceId,
                                       finfo.m_arrayIndices, VAccess::WRITE);
                AstAssign* const rhsAssignp = new AstAssign{flp, arraySelp, finfo.m_rhsExprp};
                AstAlways* const alwaysp
                    = new AstAlways{flp, VAlwaysKwd::ALWAYS, nullptr, rhsAssignp};
                AstSenTree* const senTreep
                    = new AstSenTree{flp, new AstSenItem{flp, AstSenItem::Combo{}}};
                AstActive* const activep = new AstActive{flp, "force-rhs-update", senTreep};
                activep->senTreeStorep(activep->sentreep());
                activep->addStmtsp(alwaysp);
                scopep->addBlocksp(activep);
            }
        }
    }

    ForceInfo getForceInfo(AstAssignForce* forceStmtp) const {
        AstVar* varp = getOneVarRef(forceStmtp->lhsp())->varp();
        auto it = m_varInfo.find(varp);
        UASSERT(it != m_varInfo.end(), "Force info not found for variable");
        auto it2 = it->second.m_forces.find(forceStmtp);
        UASSERT(it2 != it->second.m_forces.end(), "Force statement not found");
        return it2->second;
    }

    AstNodeExpr* createForceReadExpression(const VarForceInfo& varInfo,
                                           AstVarRef* originalRefp) const {
        FileLine* const flp = originalRefp->fileline();
        AstVar* const varp = originalRefp->varp();

        AstVarRef* const origRefp = originalRefp->cloneTreePure(false);
        markNonReplaceable(origRefp);
        AstNodeExpr* const origValp = castToVarDType(origRefp, varp);

        const string cType = varp->dtypep()->cType("", false, false);
        const string funcName
            = isUnpackedArrayDType(varp->dtypep()) ? "VlForceReadArray" : "VlForceRead";
        return createForceReadCall(varInfo, flp, funcName, cType, origValp, varp, nullptr);
    }

    AstNodeExpr* createForceReadIndexExpression(const VarForceInfo& varInfo,
                                                AstNodeExpr* originalExprp,
                                                AstNodeExpr* indexExprp) const {
        FileLine* const flp = originalExprp->fileline();
        AstNodeExpr* const origValp = originalExprp->cloneTreePure(false);
        origValp->foreach([](AstVarRef* const refp) { ForceState::markNonReplaceable(refp); });

        const string cType = originalExprp->dtypep()->cType("", false, false);
        return createForceReadCall(varInfo, flp, "VlForceReadIndex", cType, origValp,
                                   originalExprp, indexExprp);
    }
};

//######################################################################
// ForceDiscoveryVisitor - Discover force statements

class ForceDiscoveryVisitor final : public VNVisitorConst {
    ForceState& m_state;

    void visit(AstAssignForce* nodep) override {
        if (nodep->user2()) return;  // External force statements are pre-registered.
        UINFO(2, "Discovering force statement: " << nodep << "\n");

        AstVarRef* const lhsVarRefp = m_state.getOneVarRef(nodep->lhsp());
        AstVar* const forcedVarp = lhsVarRefp->varp();
        UASSERT(forcedVarp, "VarRef missing Varp");

        ForceState::ForceRangeInfo rangeInfo = ForceState::getForceRangeInfo(
            nodep->lhsp(), forcedVarp, true, true,
            "Unsupported: force on unpacked array with bit select");
        AstNodeExpr* const rhsExprp
            = ForceState::padRhsToVarWidth(nodep->rhsp()->cloneTreePure(false), forcedVarp,
                                           rangeInfo.m_padLsb, rangeInfo.m_padMsb);

        m_state.addForceAssignment(forcedVarp, lhsVarRefp->varScopep(), rhsExprp, nodep,
                                   rangeInfo.m_rangeLsb, rangeInfo.m_rangeMsb, rangeInfo.m_padLsb,
                                   rangeInfo.m_padMsb, std::move(rangeInfo.m_arrayIndices));
    }

    void visit(AstVarScope* nodep) override {
        if (nodep->varp()->isForceable()) {
            if (VN_IS(nodep->varp()->dtypeSkipRefp(), UnpackArrayDType)) {
                nodep->varp()->v3warn(
                    E_UNSUPPORTED,
                    "Unsupported: Forcing unpacked arrays: " << nodep->varp()->name());  // (#4735)
                return;
            }

            const AstBasicDType* const bdtypep = nodep->varp()->basicp();
            if (bdtypep && bdtypep->keyword() == VBasicDTypeKwd::STRING) {
                nodep->varp()->v3error(
                    "Forcing strings is not permitted: " << nodep->varp()->name());
            }

            m_state.addExternalForce(nodep, nodep->varp());
        }
        iterateChildrenConst(nodep);
    }

    void visit(AstNode* nodep) override { iterateChildrenConst(nodep); }

public:
    explicit ForceDiscoveryVisitor(AstNetlist* nodep, ForceState& state)
        : m_state{state} {
        iterateAndNextConstNull(nodep->modulesp());
    }
};

//######################################################################
// ForceConvertVisitor - Convert force/release statements

class ForceConvertVisitor final : public VNVisitor {
    ForceState& m_state;

    void visit(AstAssignForce* nodep) override {
        UINFO(2, "Converting force statement: " << nodep << "\n");

        AstNodeExpr* const lhsp = nodep->lhsp();
        AstVarRef* const lhsVarRefp = m_state.getOneVarRef(lhsp);
        AstVar* const forcedVarp = lhsVarRefp->varp();

        const ForceState::ForceInfo info = m_state.getForceInfo(nodep);
        const ForceState::VarForceInfo* const varInfo = m_state.getVarInfo(forcedVarp);
        UASSERT_OBJ(varInfo && varInfo->m_forceVecVscp, nodep, "Force info not set up");

        FileLine* const flp = nodep->fileline();

        // Assign RHS shadow value immediately so force takes effect in the same timestep.
        AstNodeExpr* const rhsExprp = ForceState::padRhsToVarWidth(
            nodep->rhsp()->cloneTreePure(false), forcedVarp, info.m_padLsb, info.m_padMsb);
        UASSERT_OBJ(varInfo->m_rhsArrayVscp, nodep, "No RHS array for forced variable");
        AstNodeExpr* const rhsArraySelp = ForceState::buildRhsArraySel(
            flp, varInfo->m_rhsArrayVscp, info.m_forceId, info.m_arrayIndices, VAccess::WRITE);
        AstAssign* const rhsAssignp = new AstAssign{flp, rhsArraySelp, rhsExprp};

        AstAssign* valAssignp = nullptr;
        AstAssign* enAssignp = nullptr;
        if (!nodep->user2() && varInfo->m_forceEnVscp && varInfo->m_forceValVscp
            && info.m_arrayIndices.empty()) {
            if (ForceState::isBitwiseDType(forcedVarp) && forcedVarp->width() > 0) {
                AstConst* const maskConstp = ForceState::makeRangeMaskConst(
                    nodep, forcedVarp->width(), info.m_rangeLsb, info.m_rangeMsb);
                AstNodeExpr* const rhsValp = rhsExprp->cloneTreePure(false);
                AstNodeExpr* const valReadp
                    = new AstVarRef{flp, varInfo->m_forceValVscp, VAccess::READ};
                AstNodeExpr* const valWritep
                    = new AstVarRef{flp, varInfo->m_forceValVscp, VAccess::WRITE};
                AstNodeExpr* const notMaskp = new AstNot{flp, maskConstp};
                AstNodeExpr* const maskedOldp = new AstAnd{flp, valReadp, notMaskp};
                AstNodeExpr* const newValp = new AstOr{flp, maskedOldp, rhsValp};
                valAssignp = new AstAssign{flp, valWritep, newValp};

                AstNodeExpr* const enReadp
                    = new AstVarRef{flp, varInfo->m_forceEnVscp, VAccess::READ};
                AstNodeExpr* const enWritep
                    = new AstVarRef{flp, varInfo->m_forceEnVscp, VAccess::WRITE};
                AstNodeExpr* const newEnp
                    = new AstOr{flp, enReadp, maskConstp->cloneTreePure(false)};
                enAssignp = new AstAssign{flp, enWritep, newEnp};
            } else {
                AstConst* const oneConstp = ForceState::makeRangeMaskConst(nodep, 1, 0, 0);
                AstNodeExpr* const rhsValp
                    = ForceState::castToVarDType(rhsExprp->cloneTreePure(false), forcedVarp);
                valAssignp = new AstAssign{
                    flp, new AstVarRef{flp, varInfo->m_forceValVscp, VAccess::WRITE}, rhsValp};
                enAssignp = new AstAssign{
                    flp, new AstVarRef{flp, varInfo->m_forceEnVscp, VAccess::WRITE}, oneConstp};
            }
        }

        // Use CStmt with VarRef - V3Order now skips CStmt children
        AstCStmt* const stmtp = new AstCStmt{flp};
        stmtp->add(new AstVarRef{flp, varInfo->m_forceVecVscp, VAccess::WRITE});
        stmtp->add(".addForce(" + std::to_string(info.m_rangeLsb) + ", "
                   + std::to_string(info.m_rangeMsb) + ", " + std::to_string(info.m_forceId)
                   + ");\n");

        AstNode* tailp = rhsAssignp;
        if (valAssignp) {
            tailp->addNextHere(valAssignp);
            tailp = valAssignp;
        }
        if (enAssignp) {
            tailp->addNextHere(enAssignp);
            tailp = enAssignp;
        }
        tailp->addNextHere(stmtp);
        nodep->replaceWith(rhsAssignp);
        VL_DO_DANGLING(pushDeletep(nodep), nodep);
    }

    void visit(AstRelease* nodep) override {
        UINFO(2, "Converting release statement: " << nodep << "\n");

        AstNodeExpr* const lhsp = nodep->lhsp();
        AstVarRef* const lhsVarRefp = m_state.getOneVarRef(lhsp);
        AstVar* const releasedVarp = lhsVarRefp->varp();

        const ForceState::VarForceInfo* const varInfo = m_state.getVarInfo(releasedVarp);
        if (!varInfo || varInfo->m_forces.empty()) {
            VL_DO_DANGLING(pushDeletep(nodep->unlinkFrBack()), nodep);
            return;
        }

        FileLine* const flp = nodep->fileline();

        const ForceState::ForceRangeInfo rangeInfo = ForceState::getForceRangeInfo(
            lhsp, releasedVarp, false, false,
            "Unsupported: release on unpacked array with bit select");

        AstCStmt* const releasep = new AstCStmt{flp};
        releasep->add(new AstVarRef{flp, varInfo->m_forceVecVscp, VAccess::WRITE});
        releasep->add(".release(" + std::to_string(rangeInfo.m_rangeLsb) + ", "
                      + std::to_string(rangeInfo.m_rangeMsb) + ");\n");

        AstAssign* clearEnp = nullptr;
        if (releasedVarp->isForceable() && varInfo->m_forceEnVscp && !rangeInfo.m_hasArraySel) {
            AstNodeExpr* enLhsp = new AstVarRef{flp, varInfo->m_forceEnVscp, VAccess::WRITE};
            int enWidth = 1;
            if (ForceState::isBitwiseDType(releasedVarp)) {
                const int varWidth = releasedVarp->width();
                if (varWidth > 0) {
                    enWidth = rangeInfo.m_rangeMsb - rangeInfo.m_rangeLsb + 1;
                    if (!(rangeInfo.m_rangeLsb == 0 && rangeInfo.m_rangeMsb == varWidth - 1)) {
                        enLhsp = new AstSel{flp, enLhsp, rangeInfo.m_rangeLsb, enWidth};
                    } else {
                        enWidth = varWidth;
                    }
                }
            }
            AstConst* const zeroConst = ForceState::makeZeroConst(nodep, enWidth);
            clearEnp = new AstAssign{flp, enLhsp, zeroConst};
        }

        const AstSel* const selp = VN_CAST(lhsp, Sel);
        AstNodeExpr* const basep = selp ? selp->fromp() : lhsp;

        AstNode* stmtListp = releasep;
        if (clearEnp) {
            clearEnp->addNextHere(stmtListp);
            stmtListp = clearEnp;
        }

        // IEEE 1800-2023 10.6.2: When released, if the variable is not continuously driven,
        // it maintains its current value until the next procedural assignment.
        if (!releasedVarp->isContinuously()) {
            AstNodeExpr* forceReadp
                = rangeInfo.m_hasArraySel
                      ? m_state.createForceReadIndexExpression(
                            *varInfo, basep,
                            ForceState::buildFlattenIndexExpr(flp, rangeInfo.m_arrayInfo))
                      : m_state.createForceReadExpression(*varInfo, lhsVarRefp);
            if (selp) {
                forceReadp = new AstSel{flp, forceReadp, selp->lsbConst(), selp->widthConst()};
            }
            AstAssign* const assignp = new AstAssign{flp, lhsp->cloneTreePure(false), forceReadp};
            assignp->addNextHere(stmtListp);
            stmtListp = assignp;
        }

        nodep->replaceWith(stmtListp);
        VL_DO_DANGLING(pushDeletep(nodep), nodep);
    }

    void visit(AstNode* nodep) override { iterateChildren(nodep); }

public:
    ForceConvertVisitor(AstNetlist* nodep, ForceState& state)
        : m_state{state} {
        iterateAndNextNull(nodep->modulesp());
    }
};

//######################################################################
// ForceReplaceVisitor - Replace variable reads with force-aware reads

class ForceReplaceVisitor final : public VNVisitor {
    const ForceState& m_state;

    void visit(AstArraySel* nodep) override {
        if (nodep->backp() && VN_IS(nodep->backp(), ArraySel)) {
            iterateChildren(nodep);
            return;
        }

        AstVarRef* const baseRefp = m_state.getOneVarRef(nodep);
        AstVar* const varp = baseRefp->varp();
        const ForceState::VarForceInfo* const varInfo = m_state.getVarInfo(varp);
        if (ForceState::isNotReplaceable(baseRefp) || !varInfo || varInfo->m_forces.empty()
            || !ForceState::isUnpackedArrayDType(varp->dtypep())
            || VN_IS(nodep->dtypep()->skipRefp(), UnpackArrayDType)) {
            iterateChildren(nodep);
            return;
        }

        if (baseRefp->access().isRW()) {
            baseRefp->v3warn(
                E_UNSUPPORTED,
                "Unsupported: Signals used via read-write reference cannot be forced");
            iterateChildren(nodep);
        } else if (baseRefp->access().isReadOnly()) {
            const ForceState::ArraySelInfo arrayInfo = ForceState::getArraySelInfo(nodep);
            AstNodeExpr* const indexExprp
                = ForceState::buildFlattenIndexExpr(nodep->fileline(), arrayInfo);
            AstNodeExpr* const readExprp
                = m_state.createForceReadIndexExpression(*varInfo, nodep, indexExprp);
            nodep->replaceWith(readExprp);
            VL_DO_DANGLING(pushDeletep(nodep), nodep);
        } else {
            iterateChildren(nodep);
        }
    }

    void visit(AstVarRef* nodep) override {
        if (ForceState::isNotReplaceable(nodep)) return;
        if (nodep->backp() && VN_IS(nodep->backp(), ArraySel)) return;

        AstVar* const varp = nodep->varp();
        const ForceState::VarForceInfo* const varInfo = m_state.getVarInfo(varp);
        if (!varInfo || varInfo->m_forces.empty()) return;

        if (nodep->access().isRW()) {
            nodep->v3warn(E_UNSUPPORTED,
                          "Unsupported: Signals used via read-write reference cannot be forced");
        } else if (nodep->access().isReadOnly()) {
            ForceState::markNonReplaceable(nodep);
            AstNodeExpr* const readExprp = m_state.createForceReadExpression(*varInfo, nodep);
            nodep->replaceWith(readExprp);
            VL_DO_DANGLING(pushDeletep(nodep), nodep);
        }
    }
    void visit(AstNode* nodep) override { iterateChildren(nodep); }

public:
    explicit ForceReplaceVisitor(AstNetlist* nodep, const ForceState& state)
        : m_state{state} {
        iterateAndNextNull(nodep->modulesp());
    }
};
//######################################################################
//

//######################################################################
// V3Force - Main entry point

void V3Force::forceAll(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":\n");
    if (!v3Global.hasForceableSignals()) return;
    ForceState state;
    { ForceDiscoveryVisitor{nodep, state}; }
    state.finalizeRhsArrays();
    { ForceConvertVisitor{nodep, state}; }
    { ForceReplaceVisitor{nodep, state}; }
    V3Global::dumpCheckGlobalTree("force", 0, dumpTreeEitherLevel() >= 3);
}
