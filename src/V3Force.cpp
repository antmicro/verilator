// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Convert forceable signals, process force/release
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
//  V3Force's Transformations:
//
//  For each forceable var/net "<name>":
//    - Create <name>__VforceVec (VlForceVec) to track active force ranges
//    - Create <name>__VforceRHS<ID> vars to hold RHS shadow values
//    - Add continuous assignments: <name>__VforceRHS<ID> = RHS
//
//  For each `force <name>[range] = <RHS>` with ID:
//    - <name>__VforceVec.addForce(lsb, msb, &__VforceRHS, rhsLsb)
//
//  For each `release <name>[range]`:
//    - If not continuously driven: <name> = VlForceRead(<name>, __VforceVec)
//    - <name>__VforceVec.release(lsb, msb)
//
//  For each read of <name>:
//    - Replace with: VlForceRead(<name>, __VforceVec)
//
//*************************************************************************

#include "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#include "V3Force.h"

#include "V3AstUserAllocator.h"
#include "V3UniqueNames.h"

VL_DEFINE_DEBUG_FUNCTIONS;

class ForceState final {
public:
    struct ForceInfo final {
        int m_forceId;
        int m_rangeLsb;  // VlForceVec range: bit index or array element index
        int m_rangeMsb;
        int m_padLsb;  // Bit positions for RHS padding
        int m_padMsb;
        bool m_hasArraySel = false;
        AstVarScope* m_rhsVarVscp = nullptr;
        AstNodeExpr* m_rhsExprp;
    };

    struct VarForceInfo final {
        AstVarScope* m_forceVecVscp = nullptr;
        AstVarScope* m_forceTokVscp = nullptr;
        AstVarScope* m_forceEnVscp = nullptr;
        AstVarScope* m_forceValVscp = nullptr;
        AstScope* m_scopep = nullptr;
        std::unordered_map<AstAssignForce*, ForceInfo> m_forces;
        std::unordered_map<string, int> m_forcePathToIndex;
        int m_nextForcePathIndex = 1;
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
                           false);
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

    static AstNodeExpr* buildArrayBitSelForceRhs(const ForceRangeInfo& rangeInfo,
                                                 AstNodeExpr* lhsp, AstNodeExpr* rhsp) {
        const AstSel* const selp = VN_CAST(lhsp, Sel);
        UASSERT_OBJ(selp, lhsp, "Missing bit select for unpacked array bit-select force");
        UASSERT_OBJ(isBitwiseDType(selp->fromp()), lhsp,
                    "Unsupported non-bitwise base for unpacked array bit-select force");

        AstNodeExpr* const baseExprp = selp->fromp()->cloneTreePure(false);
        baseExprp->foreach([](AstVarRef* const refp) { markNonReplaceable(refp); });

        AstNodeExpr* rhsExprp = rhsp->cloneTreePure(false);
        const int baseWidth = selp->fromp()->width();
        if (baseWidth > 0) {
            const int lowPad = rangeInfo.m_padLsb;
            const int highPad = baseWidth - (rangeInfo.m_padMsb + 1);
            if (lowPad > 0) {
                rhsExprp = new AstConcat{rhsExprp->fileline(), rhsExprp,
                                         makeZeroConst(rhsExprp, lowPad)};
            }
            if (highPad > 0) {
                rhsExprp = new AstConcat{rhsExprp->fileline(), makeZeroConst(rhsExprp, highPad),
                                         rhsExprp};
            }
        }

        AstConst* const maskConstp = makeRangeMaskConst(lhsp, selp->fromp()->width(),
                                                        rangeInfo.m_padLsb, rangeInfo.m_padMsb);
        AstNodeExpr* const notMaskp = new AstNot{lhsp->fileline(), maskConstp};
        AstNodeExpr* const maskedOldp = new AstAnd{lhsp->fileline(), baseExprp, notMaskp};
        AstNodeExpr* const mergedp = new AstOr{lhsp->fileline(), maskedOldp, rhsExprp};
        return mergedp;
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

    static void buildForcePathKeyRecurse(AstNodeExpr* nodep, string& out) {
        if (VN_IS(nodep, VarRef)) return;
        if (const AstSel* const selp = VN_CAST(nodep, Sel)) {
            buildForcePathKeyRecurse(selp->fromp(), out);
            out += "|S" + cvtToStr(selp->lsbConst()) + ":" + cvtToStr(selp->widthConst());
            return;
        }
        if (const AstArraySel* const selp = VN_CAST(nodep, ArraySel)) {
            buildForcePathKeyRecurse(selp->fromp(), out);
            if (const AstConst* const constp = VN_CAST(selp->bitp(), Const)) {
                out += "|A" + cvtToStr(constp->toSInt());
            } else {
                out += "|A?";
            }
            return;
        }
        if (const AstMemberSel* const selp = VN_CAST(nodep, MemberSel)) {
            buildForcePathKeyRecurse(selp->fromp(), out);
            out += "|M" + selp->name();
            return;
        }
        if (const AstStructSel* const selp = VN_CAST(nodep, StructSel)) {
            buildForcePathKeyRecurse(selp->fromp(), out);
            out += "|T" + selp->name();
            return;
        }
        out += "|?";
    }

    static string forcePathKey(AstNodeExpr* nodep) {
        string out;
        buildForcePathKeyRecurse(nodep, out);
        return out;
    }

    ForceRangeInfo getForceRangeInfo(AstNodeExpr* lhsp, AstVar* varp,
                                     bool requireConstRangeSelect) {
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

            info.m_rangeMsb = info.m_rangeLsb;
        } else if (isUnpackedArrayDType(varp->dtypep())) {
            fullUnpackedRange(varp->dtypep(), info.m_rangeLsb, info.m_rangeMsb);
        }
        if (!isBitwiseDType(varp) && !info.m_hasArraySel && !VN_IS(lhsp, VarRef)) {
            VarForceInfo& varInfo = getOrCreateVarInfo(varp);
            const auto pair = varInfo.m_forcePathToIndex.emplace(forcePathKey(lhsp),
                                                                 varInfo.m_nextForcePathIndex);
            if (pair.second) ++varInfo.m_nextForcePathIndex;
            const int index = pair.first->second;
            if (index >= 0) {
                info.m_rangeLsb = index;
                info.m_rangeMsb = index;
            }
        }
        return info;
    }

    AstNodeExpr* createForceReadCall(const VarForceInfo& varInfo, FileLine* flp,
                                     const string& funcName, const string& cType,
                                     AstNodeExpr* origValp, AstNode* dtypeFromp,
                                     AstNodeExpr* indexExprp) const {
        UASSERT(varInfo.m_forceVecVscp, "No forceVec for forced variable");

        AstCExpr* const callp = new AstCExpr{flp, AstCExpr::Pure{}};
        callp->add(funcName + "<" + cType + ">(");
        callp->add(origValp);
        callp->add(", ");
        callp->add(new AstVarRef{flp, varInfo.m_forceVecVscp, VAccess::READ});
        if (indexExprp) {
            callp->add(", ");
            callp->add(indexExprp);
        }
        if (varInfo.m_forceTokVscp) {
            // Force read depends on RHS updates via force vector pointers.
            // Make this dependency explicit for DFG through a single token.
            callp->add(", ");
            callp->add(new AstVarRef{flp, varInfo.m_forceTokVscp, VAccess::READ});
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
                            int padMsb, bool hasArraySel) {
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

            AstVar* const forceTokVarp = new AstVar{
                flp, VVarType::VAR, varp->name() + "__VforceTok", VFlagBitPacked{}, 1};
            forceTokVarp->funcLocal(false);
            forceTokVarp->isInternal(true);
            forceTokVarp->noSubst(true);
            forceTokVarp->setForcedByCode();
            varp->addNextHere(forceTokVarp);
            info.m_forceTokVscp = new AstVarScope{flp, scopep, forceTokVarp};
            scopep->addVarsp(info.m_forceTokVscp);
        }

        info.m_forces.emplace(forceStmtp, ForceInfo{forceId, rangeLsb, rangeMsb, padLsb, padMsb,
                                                    hasArraySel, nullptr, rhsExprp});

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

    static AstNodeExpr* buildFlatFirstElemSel(FileLine* flp, AstVarScope* rhsVarVscp) {
        AstNodeExpr* exprp = new AstVarRef{flp, rhsVarVscp, VAccess::READ};
        AstNodeDType* dtypep = rhsVarVscp->dtypep();
        while (const AstUnpackArrayDType* const arrayp
               = VN_CAST(dtypep->skipRefp(), UnpackArrayDType)) {
            exprp = new AstArraySel{flp, exprp, 0};
            dtypep = arrayp->subDTypep();
        }
        return exprp;
    }

    static AstNodeExpr* buildRhsDataExpr(FileLine* flp, const ForceInfo& finfo) {
        UASSERT(finfo.m_rhsVarVscp, "RHS var scope not assigned");
        if (isUnpackedArrayDType(finfo.m_rhsVarVscp->dtypep())) {
            return buildFlatFirstElemSel(flp, finfo.m_rhsVarVscp);
        }
        return new AstVarRef{flp, finfo.m_rhsVarVscp, VAccess::READ};
    }

    void finalizeRhsVars() {
        for (auto& it : m_varInfo) {
            AstVar* const varp = it.first;
            VarForceInfo& info = it.second;
            if (info.m_forces.empty()) continue;

            AstScope* const scopep
                = info.m_scopep ? info.m_scopep
                                : (info.m_forceVecVscp ? info.m_forceVecVscp->scopep() : nullptr);
            UASSERT_OBJ(scopep, varp, "Missing scope for force RHS vars");

            FileLine* const flp = varp->fileline();
            std::vector<ForceInfo*> forceps;
            forceps.reserve(info.m_forces.size());
            for (auto& fit : info.m_forces) forceps.push_back(&fit.second);
            std::sort(forceps.begin(), forceps.end(),
                      [](const ForceInfo* ap, const ForceInfo* bp) {
                          return ap->m_forceId < bp->m_forceId;
                      });

            for (ForceInfo* const finfop : forceps) {
                ForceInfo& finfo = *finfop;
                if (!finfo.m_rhsExprp) continue;
                AstVar* const rhsVarp
                    = new AstVar{flp, VVarType::VAR,
                                 varp->name() + "__VforceRHS" + std::to_string(finfo.m_forceId),
                                 finfo.m_rhsExprp->dtypep()};
                rhsVarp->noSubst(true);
                rhsVarp->sigPublic(true);
                rhsVarp->setForcedByCode();
                varp->addNextHere(rhsVarp);
                finfo.m_rhsVarVscp = new AstVarScope{flp, scopep, rhsVarp};
                scopep->addVarsp(finfo.m_rhsVarVscp);

                AstAssign* const rhsAssignp = new AstAssign{
                    flp, new AstVarRef{flp, finfo.m_rhsVarVscp, VAccess::WRITE}, finfo.m_rhsExprp};
                AstCExpr* const depExprp = new AstCExpr{flp, AstCExpr::Pure{}};
                depExprp->add("VlForceDep(");
                depExprp->add(buildRhsDataExpr(flp, finfo));
                depExprp->add(")");
                depExprp->dtypeSetLogicSized(1, VSigning::UNSIGNED);
                AstAssign* const tokAssignp = new AstAssign{
                    flp, new AstVarRef{flp, info.m_forceTokVscp, VAccess::WRITE}, depExprp};
                rhsAssignp->addNextHere(tokAssignp);
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

    const ForceInfo& getForceInfo(AstAssignForce* forceStmtp) const {
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

        ForceState::ForceRangeInfo rangeInfo
            = m_state.getForceRangeInfo(nodep->lhsp(), forcedVarp, true);
        const AstSel* const selLhsp = VN_CAST(nodep->lhsp(), Sel);
        AstNodeExpr* const rhsExprp
            = (rangeInfo.m_hasArraySel && rangeInfo.m_arrayInfo.m_hasBitSel && selLhsp
               && ForceState::isBitwiseDType(selLhsp->fromp()))
                  ? ForceState::buildArrayBitSelForceRhs(rangeInfo, nodep->lhsp(), nodep->rhsp())
                  : nodep->rhsp()->cloneTreePure(false);

        m_state.addForceAssignment(forcedVarp, lhsVarRefp->varScopep(), rhsExprp, nodep,
                                   rangeInfo.m_rangeLsb, rangeInfo.m_rangeMsb, rangeInfo.m_padLsb,
                                   rangeInfo.m_padMsb, rangeInfo.m_hasArraySel);
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

        const ForceState::ForceInfo& info = m_state.getForceInfo(nodep);
        const ForceState::VarForceInfo* const varInfo = m_state.getVarInfo(forcedVarp);
        UASSERT_OBJ(varInfo && varInfo->m_forceVecVscp, nodep, "Force info not set up");

        FileLine* const flp = nodep->fileline();

        // Assign RHS shadow value immediately so force takes effect in the same timestep.
        UASSERT_OBJ(info.m_rhsVarVscp, nodep, "No RHS var for forced variable");
        AstAssign* const rhsAssignp
            = new AstAssign{flp, new AstVarRef{flp, info.m_rhsVarVscp, VAccess::WRITE},
                            nodep->rhsp()->cloneTreePure(false)};

        AstAssign* valAssignp = nullptr;
        AstAssign* enAssignp = nullptr;
        if (!nodep->user2() && varInfo->m_forceEnVscp && varInfo->m_forceValVscp
            && !info.m_hasArraySel) {
            AstNodeExpr* const rhsPaddedExprp = ForceState::padRhsToVarWidth(
                nodep->rhsp()->cloneTreePure(false), forcedVarp, info.m_padLsb, info.m_padMsb);
            if (ForceState::isBitwiseDType(forcedVarp) && forcedVarp->width() > 0) {
                AstConst* const maskConstp = ForceState::makeRangeMaskConst(
                    nodep, forcedVarp->width(), info.m_rangeLsb, info.m_rangeMsb);
                AstNodeExpr* const valReadp
                    = new AstVarRef{flp, varInfo->m_forceValVscp, VAccess::READ};
                AstNodeExpr* const valWritep
                    = new AstVarRef{flp, varInfo->m_forceValVscp, VAccess::WRITE};
                AstNodeExpr* const notMaskp = new AstNot{flp, maskConstp};
                AstNodeExpr* const maskedOldp = new AstAnd{flp, valReadp, notMaskp};
                AstNodeExpr* const newValp = new AstOr{flp, maskedOldp, rhsPaddedExprp};
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
                    = ForceState::castToVarDType(rhsPaddedExprp, forcedVarp);
                valAssignp = new AstAssign{
                    flp, new AstVarRef{flp, varInfo->m_forceValVscp, VAccess::WRITE}, rhsValp};
                enAssignp = new AstAssign{
                    flp, new AstVarRef{flp, varInfo->m_forceEnVscp, VAccess::WRITE}, oneConstp};
            }
        }

        // Use CStmt with VarRef - V3Order now skips CStmt children
        AstCStmt* const stmtp = new AstCStmt{flp};
        AstNodeExpr* const rhsDatap = ForceState::buildRhsDataExpr(flp, info);
        stmtp->add(new AstVarRef{flp, varInfo->m_forceVecVscp, VAccess::WRITE});
        stmtp->add(".addForce(" + std::to_string(info.m_rangeLsb) + ", "
                   + std::to_string(info.m_rangeMsb) + ", &(");
        stmtp->add(rhsDatap);
        stmtp->add("), " + std::to_string(info.m_rangeLsb) + ");\n");

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

        const ForceState::ForceRangeInfo rangeInfo
            = m_state.getForceRangeInfo(lhsp, releasedVarp, false);

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
            const bool hasOpaquePath = !ForceState::isBitwiseDType(releasedVarp)
                                       && !rangeInfo.m_hasArraySel && !VN_IS(lhsp, VarRef);
            AstNodeExpr* forceReadp = nullptr;
            if (hasOpaquePath) {
                forceReadp = m_state.createForceReadIndexExpression(
                    *varInfo, lhsp, ForceState::makeConst32(flp, rangeInfo.m_rangeLsb));
            } else if (rangeInfo.m_hasArraySel) {
                forceReadp = m_state.createForceReadIndexExpression(
                    *varInfo, basep,
                    ForceState::buildFlattenIndexExpr(flp, rangeInfo.m_arrayInfo));
                if (selp) {
                    forceReadp = new AstSel{flp, forceReadp, selp->lsbConst(), selp->widthConst()};
                }
            } else {
                forceReadp = m_state.createForceReadExpression(*varInfo, lhsVarRefp);
                if (selp) {
                    forceReadp = new AstSel{flp, forceReadp, selp->lsbConst(), selp->widthConst()};
                }
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

    bool handleOpaquePathRead(AstNodeExpr* nodep) {
        AstNode* const basep = AstArraySel::baseFromp(nodep, true);
        AstVarRef* const baseRefp = VN_CAST(basep, VarRef);
        if (!baseRefp) return false;
        AstVar* const varp = baseRefp->varp();
        if (ForceState::isBitwiseDType(varp) || ForceState::isUnpackedArrayDType(varp->dtypep())) {
            return false;
        }

        const ForceState::VarForceInfo* const varInfo = m_state.getVarInfo(varp);
        if (ForceState::isNotReplaceable(baseRefp) || !varInfo || varInfo->m_forces.empty())
            return false;

        const int index = [&]() {
            const auto it = varInfo->m_forcePathToIndex.find(ForceState::forcePathKey(nodep));
            return it == varInfo->m_forcePathToIndex.end() ? -1 : it->second;
        }();
        if (index < 0) return false;

        if (baseRefp->access().isRW()) {
            baseRefp->v3warn(
                E_UNSUPPORTED,
                "Unsupported: Signals used via read-write reference cannot be forced");
            return true;
        }
        if (baseRefp->access().isReadOnly()) {
            AstNodeExpr* const readExprp = m_state.createForceReadIndexExpression(
                *varInfo, nodep, ForceState::makeConst32(nodep->fileline(), index));
            nodep->replaceWith(readExprp);
            VL_DO_DANGLING(pushDeletep(nodep), nodep);
            return true;
        }
        return false;
    }

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
        if (nodep->backp()
            && (VN_IS(nodep->backp(), Sel) || VN_IS(nodep->backp(), MemberSel)
                || VN_IS(nodep->backp(), StructSel))
            && !ForceState::isBitwiseDType(nodep->varp())
            && !ForceState::isUnpackedArrayDType(nodep->varp()->dtypep())) {
            return;
        }

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
    void visit(AstNode* nodep) override {
        if (VN_IS(nodep, Sel) || VN_IS(nodep, MemberSel) || VN_IS(nodep, StructSel)) {
            AstNode* const backp = nodep->backp();
            if ((!backp
                 || !(VN_IS(backp, Sel) || VN_IS(backp, ArraySel) || VN_IS(backp, MemberSel)
                      || VN_IS(backp, StructSel)))
                && handleOpaquePathRead(VN_AS(nodep, NodeExpr)))
                return;
        }
        iterateChildren(nodep);
    }

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
    state.finalizeRhsVars();
    { ForceConvertVisitor{nodep, state}; }
    { ForceReplaceVisitor{nodep, state}; }
    V3Global::dumpCheckGlobalTree("force", 0, dumpTreeEitherLevel() >= 3);
}
