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
    // NODE STATE
    // Cleared on Netlist
    //  AstClass::user1()       -> bool.  Set true to indicate needs randomize processing
    //  AstMemberSel::user1()   -> bool.  Set to true if already processed
    //  AstEnumDType::user2()   -> AstVar*.  Pointer to table with enum values
    // VNUser1InUse    m_inuser1;      (Allocated for use in RandomizeMarkVisitor)
    const VNUser2InUse m_inuser2;

    // STATE
    size_t m_enumValueTabCount = 0;  // Number of tables with enum values created
    AstNodeModule* m_modp;
    AstNode* m_fromp;  // Current randomized variable
    bool m_inMemberSel;  // True if iterating below a MemberSel
    bool m_hasRandVarRef;  // True if a reference to the randomized variable was found

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
        if (!nodep->user1()) return;  // Doesn't need randomize, or already processed
        UINFO(9, "Define randomize() for " << nodep << endl);
        auto* relaxp = newRelaxNextSoft(nodep);
        auto* funcp = V3Randomize::newRandomizeFunc(nodep);
        auto* genp = new AstVar(fl, VVarType::MEMBER, "constraint",
                                nodep->findBasicDType(VBasicDTypeKwd::RANDOM_GENERATOR));
        nodep->addMembersp(genp);
        auto* fvarp = VN_CAST(funcp->fvarp(), Var);

        auto* const methodp
            = new AstCMethodHard{fl, new AstVarRef{fl, genp, VAccess::READWRITE}, "next"};
        methodp->dtypeSetBit();
        funcp->addStmtsp(new AstAssign(fl, new AstVarRef(fl, fvarp, VAccess::WRITE), methodp));
        nodep->user1(false);

        auto* newp = VN_AS(nodep->findMember("new"), NodeFTask);
        UASSERT_OBJ(newp, nodep, "No new() in class");
        auto* taskp = V3Randomize::newSetupConstraintsTask(nodep);
        newp->addStmtsp(new AstTaskRef{fl, taskp->name(), nullptr});

        iterateChildren(nodep);

        nodep->foreach<AstConstraint>([&](AstConstraint* const constrp) {
            constrp->foreach<AstNodeVarRef>([&](AstNodeVarRef* const refp) {
                auto* const methodp = new AstCMethodHard{
                    fl, new AstVarRef{fl, genp, VAccess::READWRITE}, "write_var"};
                methodp->dtypep(refp->dtypep());
                refp->replaceWith(methodp);
                methodp->addPinsp(refp);
            });
            auto* condsp = constrp->condsp();
            while (condsp) {
                condsp->unlinkFrBackWithNext();
                if (auto* const softp = VN_CAST(condsp, SoftCond)) {
                    auto* const methodp
                        = new AstCMethodHard{fl, new AstVarRef{fl, genp, VAccess::READWRITE},
                                             "soft", softp->condp()->unlinkFrBack()};
                    methodp->dtypeSetVoid();
                    methodp->statement(true);
                    taskp->addStmtsp(methodp);
                    auto* delp = condsp;
                    condsp = condsp->nextp()->unlinkFrBack();
                    VL_DO_DANGLING(delp->deleteTree(), condsp);
                } else {
                    auto* const methodp = new AstCMethodHard{
                        fl, new AstVarRef{fl, genp, VAccess::READWRITE}, "hard", condsp};
                    methodp->dtypeSetVoid();
                    methodp->statement(true);
                    taskp->addStmtsp(methodp);
                    condsp = condsp->nextp();
                }
            }
        });
    }

    void visit(AstMethodCall* nodep) override {
        if (!(nodep->name() == "randomize" && nodep->pinsp())) {
            iterateChildren(nodep);
            return;
        }

        VL_RESTORER(m_inMemberSel);
        VL_RESTORER(m_fromp);

        m_fromp = nodep->fromp();
        m_inMemberSel = false;

        iterateChildren(nodep);

        auto* constrp = VN_AS(nodep->pinsp(), Constraint);
        auto* fl = constrp->fileline();

        const auto indexArgRefp = new AstLambdaArgRef{fl, "constraint__DOT__index", true};
        const auto valueArgRefp = new AstLambdaArgRef{fl, "constraint", false};
        indexArgRefp->dtypep(indexArgRefp->findCHandleDType());
        valueArgRefp->dtypep(valueArgRefp->findBasicDType(VBasicDTypeKwd::RANDOM_GENERATOR));
        const auto newp = new AstWith{fl, indexArgRefp, valueArgRefp,
                                      constrp->condsp()->unlinkFrBackWithNext()};
        newp->dtypeFrom(newp->exprp());

        auto* const classp = VN_AS(nodep->fromp()->dtypep(), ClassRefDType)->classp();

        iterate(classp);

        auto* const genp = VN_AS(classp->findMember("constraint"), Var);
        UASSERT_OBJ(genp, classp, "Randomized class without generator");
        auto* const dtypep
            = nodep->findBitDType(32, 32, VSigning::SIGNED);  // IEEE says int return of 0/1
        const auto mathp = new AstCMath{fl, nullptr};
        mathp->addExprsp(new AstText{fl, "VL_RANDOMIZE_WITH("});
        mathp->addExprsp(createRef(fl, genp, nodep->fromp(), VAccess::READWRITE));
        mathp->addExprsp(new AstText{fl, ", "});
        mathp->addExprsp(newp);
        mathp->addExprsp(new AstText{fl, ")"});
        mathp->dtypep(dtypep);

        nodep->replaceWith(mathp);
        VL_DO_DANGLING(pushDeletep(nodep), nodep);
    }

    void visit(AstMemberSel* nodep) override {
        if (nodep->user1SetOnce()) return;
        if (!m_fromp) {
            iterateChildren(nodep);
            return;
        }

        if (m_fromp->sameTree(nodep->fromp())) m_hasRandVarRef = true;
        if (m_inMemberSel) {
            iterateChildren(nodep);
            return;
        }

        VL_RESTORER(m_inMemberSel);
        m_inMemberSel = true;

        iterateChildren(nodep);

        if (m_hasRandVarRef) {
            auto* const genp = new AstLambdaArgRef{m_fromp->fileline(), "constraint", false};
            genp->dtypep(genp->findBasicDType(VBasicDTypeKwd::RANDOM_GENERATOR));
            auto* const methodp = new AstCMethodHard{nodep->fileline(), genp, "write_var"};
            methodp->dtypep(nodep->dtypep());
            nodep->replaceWith(methodp);
            methodp->addPinsp(nodep);
            m_hasRandVarRef = false;
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

AstTask* V3Randomize::newSetupConstraintsTask(AstClass* nodep) {
    auto* funcp = VN_AS(nodep->findMember("_setup_constraints"), Task);
    if (!funcp) {
        funcp = new AstTask{nodep->fileline(), "_setup_constraints", nullptr};
        funcp->classMethod(true);
        funcp->isVirtual(nodep->isExtended());
        nodep->addMembersp(funcp);
        nodep->repairCache();
    }
    return funcp;
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
