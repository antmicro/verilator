// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Expression width calculations
//
// Code available from: https://verilator.org
//
//*************************************************************************
//
// Copyright 2003-2022 by Wilson Snyder. This program is free software; you
// can redistribute it and/or modify it under the terms of either the GNU
// Lesser General Public License Version 3 or the Perl Artistic License
// Version 2.0.
// SPDX-License-Identifier: LGPL-3.0-only OR Artistic-2.0
//
//*************************************************************************
// V3Randomize's Transformations:
//
// Each randomize() method call:
//      Mark class of object on which randomize() is called
// Mark all classes that inherit from previously marked classed
// Mark all classes whose instances are randomized member variables of marked classes
// Each marked class:
//      define a virtual randomize() method that randomizes its random variables
//
//*************************************************************************

#include "config_build.h"
#include "verilatedos.h"

#include "V3Randomize.h"

#include "V3Ast.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################
// Visitor that marks classes needing a randomize() method

class RandomizeMarkVisitor final : public VNVisitor {
private:
    // NODE STATE
    // Cleared on Netlist
    //  AstClass::user1()       -> bool.  Set true to indicate needs randomize processing
    const VNUser1InUse m_inuser1;

    using DerivedSet = std::unordered_set<AstClass*>;
    using BaseToDerivedMap = std::unordered_map<AstClass*, DerivedSet>;

    BaseToDerivedMap m_baseToDerivedMap;  // Mapping from base classes to classes that extend them

    // METHODS
    void markMembers(AstClass* nodep) {
        for (auto* classp = nodep; classp;
             classp = classp->extendsp() ? classp->extendsp()->classp() : nullptr) {
            for (auto* memberp = classp->stmtsp(); memberp; memberp = memberp->nextp()) {
                // If member is rand and of class type, mark its class
                if (VN_IS(memberp, Var) && VN_AS(memberp, Var)->isRand()) {
                    if (const auto* const classRefp = VN_CAST(memberp->dtypep(), ClassRefDType)) {
                        auto* const rclassp = classRefp->classp();
                        markMembers(rclassp);
                        markDerived(rclassp);
                        rclassp->user1(true);
                    }
                }
            }
        }
    }
    void markDerived(AstClass* nodep) {
        const auto it = m_baseToDerivedMap.find(nodep);
        if (it != m_baseToDerivedMap.end()) {
            for (auto* classp : it->second) {
                classp->user1(true);
                markMembers(classp);
                markDerived(classp);
            }
        }
    }
    void markAllDerived() {
        for (const auto& p : m_baseToDerivedMap) {
            if (p.first->user1()) markDerived(p.first);
        }
    }

    // VISITORS
    void visit(AstClass* nodep) override {
        iterateChildren(nodep);
        if (nodep->extendsp()) {
            // Save pointer to derived class
            auto* const basep = nodep->extendsp()->classp();
            m_baseToDerivedMap[basep].insert(nodep);
        }
    }
    void visit(AstMethodCall* nodep) override {
        iterateChildren(nodep);
        if (nodep->name() != "randomize") return;
        if (const AstClassRefDType* const classRefp
            = VN_CAST(nodep->fromp()->dtypep(), ClassRefDType)) {
            auto* const classp = classRefp->classp();
            classp->user1(true);
            markMembers(classp);
        }
    }
    void visit(AstNode* nodep) override { iterateChildren(nodep); }

public:
    // CONSTRUCTORS
    explicit RandomizeMarkVisitor(AstNetlist* nodep) {
        iterate(nodep);
        markAllDerived();
    }
    ~RandomizeMarkVisitor() override = default;
};

//######################################################################
// Visitor that defines a randomize method where needed

class RandomizeVisitor final : public VNVisitor {
private:
    struct ConstraintSet {
        void addMinConstraint(AstNode* nodep, AstVar* varp, AstNode* valp, bool orEqual, AstVar* softp) {
            if (auto* constp = VN_CAST(valp, Const)) {
                V3Number min = constp->num();
                if (orEqual) min.opSub(constp->num(), V3Number(nodep, constp->width(), 1));
                    // Always add separate constraint - needed for relaxing
                    m_minConstraints.insert(std::make_pair(varp, std::make_pair(min, softp)));
            }
        }
        void addMaxConstraint(AstNode* nodep, AstVar* varp, AstNode* valp, bool orEqual, AstVar* softp) {
            if (auto* constp = VN_CAST(valp, Const)) {
                V3Number max = constp->num();
                if (orEqual) max.opAdd(constp->num(), V3Number(nodep, constp->width(), 1));
                    // Always add separate constraint - needed for relaxing
                    m_maxConstraints.insert(std::make_pair(varp, std::make_pair(max, softp)));
            }
        }
        void addConstraint(AstNode* nodep, AstVar* softp = nullptr) {
            if (auto* andp = VN_CAST(nodep, LogAnd)) {
                addConstraint(andp->lhsp(), softp);
                addConstraint(andp->rhsp(), softp);
            } else if (auto* biopp = VN_CAST(nodep, NodeBiop)) {
                if (auto* lhsVarp = getVarp(biopp->lhsp())) {
                    if (auto* rhsConstp = VN_CAST(biopp->rhsp(), Const)) {
                        if (VN_IS(biopp, Eq) || VN_IS(biopp, EqWild)) {
                            addMinConstraint(nodep, lhsVarp, rhsConstp, true, softp);
                            addMaxConstraint(nodep, lhsVarp, rhsConstp, true, softp);
                            return;
                        } else if (VN_IS(biopp, Gt) || VN_IS(biopp, GtS) || VN_IS(biopp, Gte)
                                   || VN_IS(biopp, GteS)) {
                            addMinConstraint(nodep, lhsVarp, rhsConstp,
                                             VN_IS(biopp, Gte) || VN_IS(biopp, GteS), softp);
                            return;
                        } else if (VN_IS(biopp, Lt) || VN_IS(biopp, LtS) || VN_IS(biopp, Lte)
                                   || VN_IS(biopp, LteS)) {
                            addMaxConstraint(nodep, lhsVarp, rhsConstp,
                                             VN_IS(biopp, Lte) || VN_IS(biopp, LteS), softp);
                            return;
                        }
                    }
                } else if (auto* rhsVarp = getVarp(biopp->rhsp())) {
                    if (auto* lhsConstp = VN_CAST(biopp->lhsp(), Const)) {
                        if (VN_IS(biopp, Eq) || VN_IS(biopp, EqWild)) {
                            addMinConstraint(nodep, rhsVarp, lhsConstp, true, softp);
                            addMaxConstraint(nodep, rhsVarp, lhsConstp, true, softp);
                            return;
                        } else if (VN_IS(biopp, Gt) || VN_IS(biopp, GtS) || VN_IS(biopp, Gte)
                                   || VN_IS(biopp, GteS)) {
                            addMaxConstraint(nodep, rhsVarp, lhsConstp,
                                             VN_IS(biopp, Gte) || VN_IS(biopp, GteS), softp);
                            return;
                        } else if (VN_IS(biopp, Lt) || VN_IS(biopp, LtS) || VN_IS(biopp, Lte)
                                   || VN_IS(biopp, LteS)) {
                            addMinConstraint(nodep, rhsVarp, lhsConstp,
                                             VN_IS(biopp, Lte) || VN_IS(biopp, LteS), softp);
                            return;
                        }
                    }
                }
            }
            nodep->v3warn(E_UNSUPPORTED, "Unsupported constraint");
        }
        AstNode* applyConstraints(AstNode* nodep, AstVar* fromp) {
            auto* fl = nodep->fileline();
            AstNode* stmtsp = nullptr;
            auto maxConstraints = m_maxConstraints;
            // For each constraint:
            // if (is_soft)
            //   varp = enabled ? constraint : varp;
            // else
            //   varp = 1 ? constraint : varp;
            for (auto c : m_minConstraints) {
                V3Number min(nodep, c.second.first.width());
                min.opAdd(c.second.first, V3Number(nodep, c.second.first.width(), 1));
                auto it = maxConstraints.find(c.first);
                AstNode* controlvarp = nullptr;
                if (auto* softvarp = c.second.second)
                    controlvarp = createRef(fl, softvarp, nullptr, VAccess::READ);
                else
                    controlvarp = new AstConst(fl, 1);
                if (it != maxConstraints.end()) {
                    auto max = it->second.first;
                    stmtsp = AstNode::addNext(
                        stmtsp, new AstAssign(
                                    fl, createRef(fl, c.first, fromp, VAccess::WRITE),
                                    new AstCond(fl, controlvarp,
                                                new AstModDiv(fl, createRef(fl, c.first, fromp, VAccess::READ),
                                                              new AstSub(fl, new AstConst(fl, max),
                                                                         new AstConst(fl, min))),
                                                createRef(fl, c.first, fromp, VAccess::WRITE))));
                    maxConstraints.erase(it);
                }
                stmtsp = AstNode::addNext(
                    stmtsp,
                    new AstAssign(fl, createRef(fl, c.first, fromp, VAccess::WRITE),
                                    new AstCond(fl, controlvarp->cloneTree(false),
                                                new AstAdd(fl, createRef(fl, c.first, fromp, VAccess::READ),
                                                           new AstConst(fl, min)),
                                                createRef(fl, c.first, fromp, VAccess::WRITE))));
            }
            for (auto c : maxConstraints) {
                AstNode* controlvarp = nullptr;
                if (auto* softvarp = c.second.second)
                    controlvarp = createRef(fl, softvarp, nullptr, VAccess::READ);
                else
                    controlvarp = new AstConst(fl, 1);
                stmtsp = AstNode::addNext(
                    stmtsp,
                    new AstAssign(fl, createRef(fl, c.first, fromp, VAccess::WRITE),
                                  new AstCond(fl, controlvarp,
                                              new AstModDivS(fl, createRef(fl, c.first, fromp, VAccess::READ),
                                                             new AstConst(fl, c.second.first)),
                                              createRef(fl, c.first, fromp, VAccess::WRITE))));
            }
            return stmtsp;
        }
        AstNode* generateCheck(AstNode* nodep, AstVar* fromp) {
            auto* fl = nodep->fileline();
            AstNode* stmtsp = new AstConst(fl, AstConst::WidthedValue(), 32, 1);
            // Soft constraint wraps the check in (!enabled || condition)
            // Expected outcome:
            // EN COND OUT
            //  0    0   1
            //  0    1   1
            //  1    0   0
            //  1    1   1
            for (auto c : m_minConstraints) {
                stmtsp = new AstAnd(fl, stmtsp,
                                    new AstGt(fl, createRef(fl, c.first, fromp, VAccess::READ),
                                              new AstConst(fl, c.second.first)));
                if (auto* softp = c.second.second) {
                    auto* softrefp = createRef(fl, softp, fromp, VAccess::READ);
                    stmtsp = new AstOr(fl, new AstNot(fl, softrefp),
                                       stmtsp);
                }
            }
            for (auto c : m_maxConstraints) {
                stmtsp = new AstAnd(fl, stmtsp,
                                    new AstLt(fl, createRef(fl, c.first, fromp, VAccess::READ),
                                              new AstConst(fl, c.second.first)));
                if (auto* softp = c.second.second) {
                    auto* softrefp = createRef(fl, softp, fromp, VAccess::READ);
                    stmtsp = new AstOr(fl, new AstNot(fl, softrefp),
                                       stmtsp);
                }
            }
            return stmtsp;
        }

        // Constraint structure: affected variable, value, reference to soft control value
        std::map<AstVar*, std::pair<V3Number, AstVar*>> m_minConstraints;
        std::map<AstVar*, std::pair<V3Number, AstVar*>> m_maxConstraints;
    };

    struct ConstraintMultiset {
        void addConstraints(AstClass* nodep) {
            for (auto* classp = nodep; classp;
                 classp = classp->extendsp() ? classp->extendsp()->classp() : nullptr) {
                addConstraints(classp->stmtsp());
            }
        }
        void addConstraints(AstNode* nodep) {
            while (nodep) {
                if (auto* constrp = VN_CAST(nodep, Constraint)) {
                    for (auto* condp = constrp->condsp(); condp; condp = condp->nextp()) {
                        if (auto* softp = VN_CAST(condp, SoftCond)) {
                            static size_t m_softConstraintCount;  // Number of soft constraint
                                                                  // control varaibles created
                            // TODO: Create control variable for relaxing
                            auto* const vardtypep = nodep->findBitDType(
                                32, 32, VSigning::SIGNED);  // use int return of 0/1
                            AstVar* const varp = new AstVar(
                                nodep->fileline(), VVarType::MODULETEMP,
                                "__Vsoft_" + cvtToStr(m_softConstraintCount++), vardtypep);
                            nodep = AstNode::addNext(nodep, varp);
                            condp = softp->condsp();
                        }
                        addConstraint(condp);
                    }
                }
                nodep = nodep->nextp();
            }
        }
        void addConstraint(AstNode* nodep, AstVar* softp = nullptr) {
            auto* biopp = VN_CAST(nodep, NodeBiop);
            if (VN_IS(nodep, And) || VN_IS(nodep, LogAnd)) {
                addConstraint(biopp->lhsp(), softp);
                addConstraint(biopp->rhsp(), softp);
            } else if (VN_IS(nodep, Or) | VN_IS(nodep, LogOr)) {
                ConstraintMultiset constraintsCopy = *this;
                addConstraint(biopp->lhsp(), softp);
                constraintsCopy.addConstraint(biopp->rhsp(), softp);
                m_constraintSets.insert(m_constraintSets.end(),
                                        constraintsCopy.m_constraintSets.begin(),
                                        constraintsCopy.m_constraintSets.end());
            } else {
                for (auto& constraintSet : m_constraintSets) {
                    constraintSet.addConstraint(nodep, softp);
                }
            }
        }
        AstNode* applyConstraints(AstNode* nodep, AstVar* fromp, size_t& varCnt) {
            if (m_constraintSets.empty()) return nullptr;
            if (m_constraintSets.size() == 1)
                return m_constraintSets[0].applyConstraints(nodep, fromp);
            auto* fl = nodep->fileline();
            AstCaseItem* casesp = nullptr;
            uint32_t i = 0;
            for (auto& constraintSet : m_constraintSets) {
                casesp = AstNode::addNext(
                    casesp, new AstCaseItem(fl, new AstConst(fl, i++),
                                            constraintSet.applyConstraints(nodep, fromp)));
            }
            auto* maxp = new AstConst(fl, m_constraintSets.size());
            auto* randVarp
                = new AstVar(nodep->fileline(), VVarType::MEMBER,
                             "__Vtemp_randomize" + std::to_string(varCnt++), maxp->dtypep());
            randVarp->funcLocal(true);
            AstNode* stmtsp = randVarp;
            auto* modp = new AstModDiv(fl, new AstRand(fl, nullptr, false), maxp);
            modp->dtypep(maxp->dtypep());
            modp->lhsp()->dtypep(maxp->dtypep());
            stmtsp->addNext(new AstAssign(fl, new AstVarRef(fl, randVarp, VAccess::WRITE), modp));
            stmtsp->addNext(new AstCase(fl, VCaseType::CT_CASE,
                                        new AstVarRef(fl, randVarp, VAccess::READ), casesp));
            return stmtsp;
        }
        AstNode* generateCheck(AstNode* nodep, AstVar* fromp) {
            auto* fl = nodep->fileline();
            if (m_constraintSets.empty()) return new AstConst(fl, AstConst::WidthedValue(), 32, 1);
            AstNode* stmtsp = nullptr;
            for (auto constraintSet : m_constraintSets) {
                auto* checkp = constraintSet.generateCheck(nodep, fromp);
                if (stmtsp)
                    stmtsp = new AstOr(fl, stmtsp, checkp);
                else
                    stmtsp = checkp;
            }
            return stmtsp;
        }

        std::vector<ConstraintSet> m_constraintSets = {ConstraintSet()};
    };

    // NODE STATE
    // Cleared on Netlist
    //  AstClass::user1()       -> bool.  Set true to indicate needs randomize processing
    //  AstEnumDType::user2()   -> AstVar*.  Pointer to table with enum values
    // VNUser1InUse    m_inuser1;      (Allocated for use in RandomizeMarkVisitor)
    const VNUser2InUse m_inuser2;

    // STATE
    size_t m_enumValueTabCount = 0;  // Number of tables with enum values created
    size_t m_funcCnt = 0;
    size_t m_varCnt = 0;
    AstNodeModule* m_modp;
    ConstraintMultiset m_constraints;

    // METHODS
    AstVar* enumValueTabp(AstEnumDType* nodep) {
        if (nodep->user2p()) return VN_AS(nodep->user2p(), Var);
        UINFO(9, "Construct Venumvaltab " << nodep << endl);
        AstNodeArrayDType* const vardtypep
            = new AstUnpackArrayDType(nodep->fileline(), nodep->dtypep(),
                                      new AstRange(nodep->fileline(), nodep->itemCount(), 0));
        AstInitArray* const initp = new AstInitArray(nodep->fileline(), vardtypep, nullptr);
        v3Global.rootp()->typeTablep()->addTypesp(vardtypep);
        AstVar* const varp
            = new AstVar(nodep->fileline(), VVarType::MODULETEMP,
                         "__Venumvaltab_" + cvtToStr(m_enumValueTabCount++), vardtypep);
        varp->isConst(true);
        varp->isStatic(true);
        varp->valuep(initp);
        // Add to root, as don't know module we are in, and aids later structure sharing
        v3Global.rootp()->dollarUnitPkgAddp()->addStmtsp(varp);
        UASSERT_OBJ(nodep->itemsp(), nodep, "Enum without items");
        for (AstEnumItem* itemp = nodep->itemsp(); itemp;
             itemp = VN_AS(itemp->nextp(), EnumItem)) {
            AstConst* const vconstp = VN_AS(itemp->valuep(), Const);
            UASSERT_OBJ(vconstp, nodep, "Enum item without constified value");
            initp->addValuep(vconstp->cloneTree(false));
        }
        nodep->user2p(varp);
        return varp;
    }

    AstFunc* newRelaxNextSoft(AstClass* nodep) {
        auto* funcp = VN_AS(nodep->findMember("relax_next"), Func);
        if (!funcp) {
            auto fl = nodep->fileline();
            auto* const dtypep
                = nodep->findBitDType(32, 32, VSigning::SIGNED);  // use int return of 0/1
            auto* const fvarp
                = new AstVar(nodep->fileline(), VVarType::MEMBER, "relax_next", dtypep);
            fvarp->lifetime(VLifetime::AUTOMATIC);
            fvarp->funcLocal(true);
            fvarp->funcReturn(true);
            fvarp->direction(VDirection::OUTPUT);

            funcp = new AstFunc(fl, "relax_next", nullptr, fvarp);
            funcp->dtypep(dtypep);
            funcp->classMethod(true);
            funcp->isVirtual(nodep->isExtended());
            funcp->addStmtsp(new AstAssign(fl, createRef(fl, fvarp, nullptr, VAccess::WRITE),
                                           new AstConst(fl, 0)));

            for (auto* memberp = nodep->stmtsp(); memberp; memberp = memberp->nextp()) {
                auto* const memberVarp = VN_CAST(memberp, Var);
                if (!memberVarp || (memberVarp->name().find("__Vsoft") == std::string::npos))
                    continue;
                auto* varrefp = createRef(fl, memberVarp, nullptr, VAccess::READWRITE);
                // For each soft constraint control variable generate:
                // if (VsoftX != 0) {
                //   VsoftX = 0;
                //   return 1; // relaxed something
                // }
                // TODO: Look at constraint priorities
                auto* condp = new AstNeq(fl, varrefp, new AstConst(fl, 0));
                varrefp = varrefp->cloneTree(false);
                auto* stmtsp = new AstBegin(
                    fl, "", new AstAssign(fl, varrefp, new AstConst(fl, 0)), false, false);
                stmtsp->addStmtsp(new AstReturn(fl, new AstConst(fl, 1)));
                // stmtsp->addStmtsp(new AstAssign(fl, createRef(fl, fvarp, nullptr,
                // VAccess::WRITE), new AstConst(fl, 1))); // Relaxed something
                auto* ifp = new AstIf(fl, condp, stmtsp);
                funcp->addStmtsp(ifp);
            }
            funcp->addStmtsp(new AstReturn(fl, new AstConst(fl, 0)));  // Nothing left to relax

            nodep->addMembersp(funcp);
            nodep->repairCache();
        }
        return funcp;
    }

    AstNodeStmt* newRandStmtsp(FileLine* fl, AstNode* varrefp, int offset = 0,
                               AstMemberDType* memberp = nullptr) {
        if (const auto* const structDtp
            = VN_CAST(memberp ? memberp->subDTypep()->skipRefp() : varrefp->dtypep()->skipRefp(),
                      StructDType)) {
            AstNodeStmt* stmtsp = nullptr;
            offset += memberp ? memberp->lsb() : 0;
            for (auto* smemberp = structDtp->membersp(); smemberp;
                 smemberp = VN_AS(smemberp->nextp(), MemberDType)) {
                auto* const randp = newRandStmtsp(fl, stmtsp ? varrefp->cloneTree(false) : varrefp,
                                                  offset, smemberp);
                if (stmtsp) {
                    stmtsp->addNext(randp);
                } else {
                    stmtsp = randp;
                }
            }
            return stmtsp;
        } else {
            AstNodeMath* valp;
            if (auto* const enumDtp = VN_CAST(memberp ? memberp->subDTypep()->subDTypep()
                                                      : varrefp->dtypep()->subDTypep(),
                                              EnumDType)) {
                AstVarRef* const tabRefp
                    = new AstVarRef(fl, enumValueTabp(enumDtp), VAccess::READ);
                tabRefp->classOrPackagep(v3Global.rootp()->dollarUnitPkgAddp());
                auto* const randp = new AstRand(fl, nullptr, false);
                auto* const moddivp
                    = new AstModDiv(fl, randp, new AstConst(fl, enumDtp->itemCount()));
                randp->dtypep(varrefp->findBasicDType(VBasicDTypeKwd::UINT32));
                moddivp->dtypep(enumDtp);
                valp = new AstArraySel(fl, tabRefp, moddivp);
            } else {
                valp = new AstRand(fl, nullptr, false);
                valp->dtypep(memberp ? memberp->dtypep() : varrefp->dtypep());
            }
            return new AstAssign(fl,
                                 new AstSel(fl, varrefp, offset + (memberp ? memberp->lsb() : 0),
                                            memberp ? memberp->width() : varrefp->width()),
                                 valp);
        }
    }

    // VISITORS
    AstNode* newClassRandStmtsp(AstClass* nodep, AstNode* fromp) {
        AstNode* stmtsp = nullptr;
        for (auto* classp = nodep; classp;
             classp = classp->extendsp() ? classp->extendsp()->classp() : nullptr) {
            for (auto* memberp = classp->stmtsp(); memberp; memberp = memberp->nextp()) {
                auto* const memberVarp = VN_CAST(memberp, Var);
                // TODO: if soft constraint control add relaxing
                if (!memberVarp)
                    continue;
                else if (memberVarp->name().find("__Vsoft") == std::string::npos) {
                    // stmtsp = AstNode::addNext(stmtsp, newRelaxNextSoft(nodep));
                    newRelaxNextSoft(nodep);
                } else if (!memberVarp->isRand())
                    continue;
                const auto* const dtypep = memberp->dtypep()->skipRefp();
                if (VN_IS(dtypep, BasicDType) || VN_IS(dtypep, StructDType)) {
                    auto* const refp
                        = createRef(nodep->fileline(), memberVarp, fromp, VAccess::WRITE);
                    stmtsp = AstNode::addNext(stmtsp, newRandStmtsp(nodep->fileline(), refp));
                } else if (const auto* const classRefp = VN_CAST(dtypep, ClassRefDType)) {
                    auto* const refp
                        = new AstVarRef(nodep->fileline(), memberVarp, VAccess::WRITE);
                    auto* const memberFuncp = V3Randomize::newTryRandFunc(classRefp->classp());
                    stmtsp = AstNode::addNext(
                        stmtsp, newClassRandStmtsp(classRefp->classp(),
                                                   createRef(nodep->fileline(), memberVarp, fromp,
                                                             VAccess::WRITE)));
                } else {
                    memberp->v3warn(E_UNSUPPORTED,
                                    "Unsupported: random member variables with type "
                                        << memberp->dtypep()->prettyDTypeNameQ());
                }
            }
        }
        return stmtsp;
    }
    static AstVar* getVarp(AstNode* nodep) {
        AstVar* varp = nullptr;
        if (auto* varrefp = VN_CAST(nodep, VarRef))
            varp = varrefp->varp();
        else if (auto* extendp = VN_CAST(nodep, Extend))
            varp = VN_CAST(extendp->lhsp(), VarRef)->varp();
        else if (auto* memberSelp = VN_CAST(nodep, MemberSel))
            varp = memberSelp->varp();
        return varp;
    }
    static AstNode* createRef(FileLine* fl, AstVar* varp, AstNode* fromp, VAccess access) {
        if (fromp) {
            AstMemberSel* memberSelp = nullptr;
            if (auto* fromMemberSelp = VN_CAST(fromp, MemberSel)) {
                memberSelp = new AstMemberSel(fl, fromMemberSelp->cloneTree(false),
                                              VFlagChildDType(), varp->name());
            } else if (auto* fromVarRefp = VN_CAST(fromp, VarRef)) {
                memberSelp = new AstMemberSel(fl, fromVarRefp->cloneTree(false), VFlagChildDType(),
                                              varp->name());
            } else if (auto* fromVarp = VN_CAST(fromp, Var)) {
                memberSelp = new AstMemberSel(fl, new AstVarRef(fl, fromVarp, access),
                                              VFlagChildDType(), varp->name());
            }
            memberSelp->varp(varp);
            memberSelp->dtypep(varp->dtypep());
            return memberSelp;
        }
        return new AstVarRef(fl, varp, access);
    }

    // VISITORS
    void visit(AstClass* nodep) override {
        auto* fl = nodep->fileline();
        VL_RESTORER(m_modp);
        m_modp = nodep;
        m_constraints.addConstraints(nodep);
        if (!nodep->user1()) return;  // Doesn't need randomize, or already processed
        UINFO(9, "Define randomize() for " << nodep << endl);
        auto* relaxp = newRelaxNextSoft(nodep);
        auto* parentfuncp = V3Randomize::newRandomizeFunc(nodep);
        auto* funcp = V3Randomize::newTryRandFunc(nodep);
        auto* fvarp = VN_CAST(funcp->fvarp(), Var);
        auto* rvarp = VN_CAST(relaxp->fvarp(), Var);

        funcp->addStmtsp(newClassRandStmtsp(nodep, nullptr));
        funcp->addStmtsp(m_constraints.applyConstraints(funcp, nullptr, m_varCnt));
        funcp->addStmtsp(new AstAssign(fl, new AstVarRef(fl, fvarp, VAccess::WRITE),
                                       m_constraints.generateCheck(funcp, nullptr)));
        // if (randomize() == 0) {
        //   while (relax_next()) { // As long as something could be relaxed
        //     if (randomize()) // retry
        //       break;  // success
        //   }
        // }
        //
        auto* dtypep = nodep->findBitDType(32, 32, VSigning::SIGNED);
        auto* const frefp
            = new AstVarRef(nodep->fileline(), fvarp, VAccess::WRITE);
        auto* randcallp = new AstFuncRef(fl, "try_rand", nullptr);
        randcallp->taskp(funcp);
        randcallp->dtypeFrom(funcp);
        auto* const rrefp
            = new AstVarRef(nodep->fileline(), rvarp, VAccess::WRITE);
        auto* relaxcallp = new AstFuncRef(fl, "relax_next", nullptr);
        auto* rdtypep = nodep->findBitDType(32, 32, VSigning::SIGNED);
        relaxcallp->taskp(relaxp);
        relaxcallp->dtypeFrom(relaxp);
        auto* redop = new AstWhile(fl, relaxcallp,
                                   new AstIf(fl,
                                             new AstNeq(fl,
                                       new AstRedOr(fl, randcallp),
                                                       new AstConst(fl, 0)),
                                             new AstBreak(fl)));

        auto* ifp = new AstIf(fl, new AstNeq(fl,
                                       new AstRedOr(fl, frefp->cloneTree(false)),
                    new AstConst(fl, 1)),
                              redop,
                              nullptr
                              );
        parentfuncp->addStmtsp(ifp);
        m_constraints = {};
        nodep->user1(false);
    }

    void visit(AstMethodCall* nodep) override {
        auto* fl = nodep->fileline();
        auto* classp = VN_CAST(VN_CAST(nodep->fromp(), VarRef)->dtypep(), ClassRefDType)->classp();
        if (nodep->name() == "randomize" && nodep->pinsp()) {
            m_constraints.addConstraints(classp);
            m_constraints.addConstraints(nodep->pinsp());
            auto* pinsp = nodep->pinsp()->unlinkFrBack();
            VL_DO_DANGLING(pinsp->deleteTree(), pinsp);
            auto* stmtsp = m_constraints.applyConstraints(
                nodep, VN_CAST(nodep->fromp(), VarRef)->varp(), m_varCnt);
            if (stmtsp) {
                std::string funcName = "__Vrandomize" + std::to_string(m_funcCnt++);
                auto* dtypep = nodep->findBitDType(32, 32, VSigning::SIGNED);
                auto* fvarp = new AstVar(fl, VVarType::MEMBER, funcName, dtypep);
                fvarp->lifetime(VLifetime::AUTOMATIC);
                fvarp->funcLocal(true);
                fvarp->funcReturn(true);
                fvarp->direction(VDirection::OUTPUT);
                auto* funcp = new AstFunc(fl, funcName, nullptr, fvarp);
                auto* refp = new AstFuncRef(fl, funcName, nullptr);
                auto* fromp = VN_CAST(nodep->fromp(), VarRef)->varp();
                funcp->addStmtsp(newClassRandStmtsp(classp, fromp));
                funcp->addStmtsp(stmtsp);
                funcp->dtypep(dtypep);
                refp->taskp(funcp);
                refp->dtypep(dtypep);
                if (auto* classp = VN_CAST(m_modp, Class)) {
                    funcp->classMethod(true);
                    classp->addMembersp(funcp);
                    classp->repairCache();
                } else {
                    m_modp->addStmtsp(funcp);
                }
                nodep->replaceWith(refp);
                VL_DO_DANGLING(nodep->deleteTree(), nodep);
            }
            m_constraints = {};
        // TODO: visit relax_next
        }
    }
    void visit(AstNodeModule* nodep) override {
        VL_RESTORER(m_modp);
        m_modp = nodep;
        iterateChildren(nodep);
    }
    void visit(AstNode* nodep) override { iterateChildren(nodep); }

public:
    // CONSTRUCTORS
    explicit RandomizeVisitor(AstNetlist* nodep) { iterate(nodep); }
    ~RandomizeVisitor() override = default;
};

//######################################################################
// Randomize method class functions

void V3Randomize::randomizeNetlist(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ": " << endl);
    {
        const RandomizeMarkVisitor markVisitor{nodep};
        RandomizeVisitor{nodep};
    }
    V3Global::dumpCheckGlobalTree("randomize", 0, dumpTree() >= 3);
}

AstFunc* V3Randomize::newRandomizeFunc(AstClass* nodep) {
    auto* funcp = VN_AS(nodep->findMember("randomize"), Func);
    if (!funcp) {
        auto* const dtypep
            = nodep->findBitDType(32, 32, VSigning::SIGNED);  // IEEE says int return of 0/1
        auto* const fvarp = new AstVar(nodep->fileline(), VVarType::MEMBER, "randomize", dtypep);
        fvarp->lifetime(VLifetime::AUTOMATIC);
        fvarp->funcLocal(true);
        fvarp->funcReturn(true);
        fvarp->direction(VDirection::OUTPUT);
        funcp = new AstFunc(nodep->fileline(), "randomize", nullptr, fvarp);
        funcp->dtypep(dtypep);
        funcp->classMethod(true);
        funcp->isVirtual(nodep->isExtended());
        nodep->addMembersp(funcp);
        nodep->repairCache();
    }
    return funcp;
}

AstFunc* V3Randomize::newTryRandFunc(AstClass* nodep) {
    // Internal randomization, used for rerunning
    auto* funcp = VN_AS(nodep->findMember("try_rand"), Func);
    if (!funcp) {
        auto* const dtypep
            = nodep->findBitDType(32, 32, VSigning::SIGNED);  // IEEE says int return of 0/1
        auto* const fvarp = new AstVar(nodep->fileline(), VVarType::MEMBER, "randomize", dtypep);
        fvarp->lifetime(VLifetime::AUTOMATIC);
        fvarp->funcLocal(true);
        fvarp->funcReturn(true);
        fvarp->direction(VDirection::OUTPUT);
        funcp = new AstFunc(nodep->fileline(), "try_rand", nullptr, fvarp);
        funcp->dtypep(dtypep);
        funcp->classMethod(true);
        funcp->isVirtual(nodep->isExtended());
        nodep->addMembersp(funcp);
        nodep->repairCache();
    }
    return funcp;
}
