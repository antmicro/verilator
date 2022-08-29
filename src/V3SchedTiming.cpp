// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Code scheduling
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
//
// Functions defined in this file are used by V3Sched.cpp to properly integrate
// static scheduling with timing features. They create external domains for
// variables, remap them to trigger vectors, and create timing resume/commit
// calls for the global eval loop. There is also a function that transforms
// forks into emittable constructs.
//
// See the internals documentation docs/internals.rst for more details.
//
//*************************************************************************

#include "config_build.h"
#include "verilatedos.h"

#include "V3EmitCBase.h"
#include "V3Sched.h"

#include <unordered_map>

namespace V3Sched {

namespace {

//============================================================================
// Utility functions

AstCMethodHard* makeCMethodHard(FileLine* const flp, AstVarScope* const fromp,
                                const char* const name, AstNode* const pinsp) {
    return new AstCMethodHard{flp, new AstVarRef{flp, fromp, VAccess::READWRITE}, name, pinsp};
}

AstCMethodHard* makeCMethodHardVoid(FileLine* const flp, AstVarScope* const fromp,
                                    const char* const name, AstNode* const pinsp = nullptr) {
    AstCMethodHard* const methodp = makeCMethodHard(flp, fromp, name, pinsp);
    methodp->dtypeSetVoid();
    methodp->statement(true);
    return methodp;
}

AstCMethodHard* makeCMethodHardBit(FileLine* const flp, AstVarScope* const fromp,
                                   const char* const name, AstNode* const pinsp = nullptr) {
    AstCMethodHard* const methodp = makeCMethodHard(flp, fromp, name, pinsp);
    methodp->dtypeSetBit();
    return methodp;
}

AstActive* convertToCommitActive(AstActive* activep) {
    auto* const resumep = VN_AS(activep->stmtsp(), CMethodHard);
    UASSERT_OBJ(!resumep->nextp(), resumep, "Should be the only statement here");
    AstVarScope* const schedulerp = VN_AS(resumep->fromp(), VarRef)->varScopep();
    UASSERT_OBJ(VN_IS(schedulerp->dtypep(), CDType)
                    && VN_AS(schedulerp->dtypep(), CDType)->isTriggerScheduler(),
                schedulerp, "Unexpected type");
    AstSenTree* const sensesp = activep->sensesp();
    FileLine* const flp = sensesp->fileline();
    // Negate the sensitivity. We will commit only if the event wasn't
    // triggered on the current iteration
    auto* const negSensesp = sensesp->cloneTree(false);
    negSensesp->sensesp()->sensp(
        new AstLogNot{flp, negSensesp->sensesp()->sensp()->unlinkFrBack()});
    sensesp->addNextHere(negSensesp);
    auto* const newactp = new AstActive{flp, "", negSensesp};
    // Create the commit call and put it in the commit function
    newactp->addStmtsp(
        makeCMethodHardVoid(flp, schedulerp, "commit", resumep->pinsp()->cloneTree(false)));
    return newactp;
}

AstCFunc* makeClassMethod(AstScope* scopep, const string& name) {
    AstCFunc* const funcp = new AstCFunc{scopep->fileline(), name, scopep, ""};
    funcp->isStatic(false);
    funcp->isConst(false);
    funcp->isMethod(true);
    funcp->argTypes(EmitCBaseVisitor::symClassVar());
    scopep->addActivep(funcp);
    return funcp;
}

}  // namespace

//============================================================================
// Remaps external domains using the specified trigger map

std::map<const AstVarScope*, std::vector<AstSenTree*>>
TimingKit::remapDomains(const std::unordered_map<const AstSenTree*, AstSenTree*>& trigMap) const {
    std::map<const AstVarScope*, std::vector<AstSenTree*>> remappedDomainMap;
    for (const auto& vscpDomains : m_externalDomains) {
        const AstVarScope* const vscp = vscpDomains.first;
        const auto& domains = vscpDomains.second;
        auto& remappedDomains = remappedDomainMap[vscp];
        remappedDomains.reserve(domains.size());
        for (AstSenTree* const domainp : domains) {
            remappedDomains.push_back(trigMap.at(domainp));
        }
    }
    return remappedDomainMap;
}

//============================================================================
// Creates a timing resume call (if needed, else returns null)

AstCCall* TimingKit::createResume(AstNetlist* const netlistp) {
    if (!m_resumeFuncp) {
        if (m_lbs.empty() && m_classLbs.empty()) return nullptr;
        // Create global resume function
        AstScope* const scopeTopp = netlistp->topScopep()->scopep();
        m_resumeFuncp = new AstCFunc{netlistp->fileline(), "_timing_resume", scopeTopp, ""};
        m_resumeFuncp->dontCombine(true);
        m_resumeFuncp->isLoose(true);
        m_resumeFuncp->isConst(false);
        m_resumeFuncp->declPrivate(true);
        scopeTopp->addActivep(m_resumeFuncp);
        for (auto& p : m_lbs) {
            // Put all the timing actives in the resume function
            AstActive* const activep = p.second;
            m_resumeFuncp->addStmtsp(activep);
        }
        const VNUser1InUse user1InUse;  // AstScope -> AstCFunc: resume func for scope
        for (auto& p : m_classLbs) {
            AstScope* const scopep = p.first;
            if (!scopep->user1p()) scopep->user1p(makeClassMethod(scopep, "_timing_resume"));
            auto* const resumeMethodp = VN_AS(scopep->user1p(), CFunc);
            AstActive* const activep = p.second;
            resumeMethodp->addStmtsp(activep);
        }
    }
    return new AstCCall{m_resumeFuncp->fileline(), m_resumeFuncp};
}

//============================================================================
// Creates a timing commit call (if needed, else returns null)

AstCCall* TimingKit::createCommit(AstNetlist* const netlistp) {
    if (!m_commitFuncp) {
        if (!m_lbs.empty() || m_classCommitp) {
            AstScope* const scopeTopp = netlistp->topScopep()->scopep();
            m_commitFuncp = new AstCFunc{netlistp->fileline(), "_timing_commit", scopeTopp, ""};
            m_commitFuncp->dontCombine(true);
            m_commitFuncp->isLoose(true);
            m_commitFuncp->isConst(false);
            m_commitFuncp->declPrivate(true);
            scopeTopp->addActivep(m_commitFuncp);
        } else {
            return nullptr;
        }
        for (auto& p : m_lbs) {
            AstActive* const activep = p.second;
            auto* const resumep = VN_AS(activep->stmtsp(), CMethodHard);
            AstVarScope* const fromp = VN_AS(resumep->fromp(), VarRef)->varScopep();
            if (!VN_AS(fromp->dtypep(), CDType)->isTriggerScheduler()) continue;
            // Create the global commit function only if we have trigger schedulers
            m_commitFuncp->addStmtsp(convertToCommitActive(activep));
        }
        const VNUser1InUse user1InUse;  // AstScope -> AstCFunc: resume func for scope
        for (auto& p : m_classLbs) {
            AstScope* const scopep = p.first;
            if (!scopep->user1p()) scopep->user1p(makeClassMethod(scopep, "_timing_commit"));
            auto* const commitMethodp = VN_AS(scopep->user1p(), CFunc);
            AstActive* const activep = p.second;
            commitMethodp->addStmtsp(convertToCommitActive(activep));
        }
        if (m_classCommitp) m_commitFuncp->addStmtsp(m_classCommitp);
    }
    return new AstCCall{m_commitFuncp->fileline(), m_commitFuncp};
}

//============================================================================
// Creates the timing kit and marks variables written by suspendables

TimingKit prepareTiming(AstNetlist* const netlistp) {
    if (!v3Global.usesTiming()) return {};
    class AwaitVisitor final : public VNVisitor {
    private:
        // NODE STATE
        //  AstSenTree::user1()  -> bool.  Set true if the sentree has been visited.
        //  AstClass::user1p()   -> AstVarScope*.  VlClassTriggerScheduler instance for this class.
        const VNUser1InUse m_inuser1;

        // STATE
        bool m_inProcess = false;  // Are we in a process?
        bool m_gatherVars = false;  // Should we gather vars in m_writtenBySuspendable?
        AstNetlist* const m_netlistp;  // Root node
        AstScope* const m_scopeTopp;  // Scope at the top
        AstClass* m_classp = nullptr;  // Current class
        AstScope* m_scopep = nullptr;  // Current scope
        AstSenItem* m_classSenItemsp = nullptr;
        LogicByScope& m_lbs;  // Timing resume actives
        LogicByScope& m_classLbs;  // Same, but for classes
        AstCMethodHard*& m_classCommitp;  // Commit calls for class trigger schedulers
        AstCMethodHard*& m_classUpdatep;  // Update calls for class trigger schedulers
        AstCMethodHard* m_classResumep = nullptr;
        // Additional var sensitivities
        std::map<const AstVarScope*, std::set<AstSenTree*>>& m_externalDomains;
        std::set<AstSenTree*> m_processDomains;  // Sentrees from the current process
        // Variables written by suspendable processes
        std::vector<AstVarScope*> m_writtenBySuspendable;

        // METHODS
        // Create an active with a timing scheduler resume() call
        void createResumeActive(AstCAwait* const awaitp) {
            auto* const methodp = VN_AS(awaitp->exprp(), CMethodHard);
            AstVarScope* const schedulerp = VN_AS(methodp->fromp(), VarRef)->varScopep();
            AstSenTree* const sensesp = awaitp->sensesp();
            FileLine* const flp = sensesp->fileline();
            // Create a resume() call on the timing scheduler
            auto* const resumep = makeCMethodHardVoid(flp, schedulerp, "resume");
            if (VN_AS(schedulerp->dtypep(), CDType)->isTriggerScheduler()) {
                resumep->addPinsp(methodp->pinsp()->cloneTree(false));
            }
            if (m_classp && VN_AS(schedulerp->dtypep(), CDType)->isTriggerScheduler()) {
                m_classLbs.add(m_scopep, sensesp, resumep);
            } else {
                m_lbs.add(m_scopep, sensesp, resumep);
            }
        }
        // Creates the current class scheduler if it doesn't exist already
        AstVarScope* getCreateClassTriggerScheduler() {
            UASSERT(m_classp, "Not under class");
            if (!m_classp->user1p()) {
                auto* const scopep = m_classp->classOrPackagep()->find<AstScope>();
                auto* const dtypep
                    = new AstCDType{scopep->fileline(), AstCDType::CLASS_TRIGGER_SCHEDULER};
                dtypep->addParam(EmitCBaseVisitor::prefixNameProtect(m_classp));
                dtypep->addParam(EmitCBaseVisitor::symClassName());
                m_netlistp->typeTablep()->addTypesp(dtypep);
                AstVarScope* const schedulerp = scopep->createTemp("__Vm_scheduler", dtypep);
                schedulerp->varp()->combineType(VVarType::MEMBER);
                m_classp->user1p(schedulerp);
            }
            return VN_AS(m_classp->user1p(), VarScope);
        }

        // VISITORS
        virtual void visit(AstNetlist* nodep) override {
            iterateChildren(nodep);
            if (m_classSenItemsp) {
                FileLine* const flp = nodep->fileline();
                auto* const sensesp = new AstSenTree{flp, m_classSenItemsp};
                nodep->topScopep()->addSenTreep(sensesp);
                m_lbs.add(m_scopeTopp, sensesp, m_classResumep);
            }
        }
        virtual void visit(AstNodeProcedure* const nodep) override {
            UASSERT_OBJ(!m_inProcess && !m_gatherVars && m_processDomains.empty()
                            && m_writtenBySuspendable.empty(),
                        nodep, "Process in process?");
            m_inProcess = true;
            m_gatherVars = nodep->isSuspendable();  // Only gather vars in a suspendable
            const VNUser2InUse user2InUse;  // AstVarScope -> bool: Set true if var has been added
                                            // to m_writtenBySuspendable
            iterateChildren(nodep);
            for (AstVarScope* const vscp : m_writtenBySuspendable) {
                m_externalDomains[vscp].insert(m_processDomains.begin(), m_processDomains.end());
                vscp->varp()->setWrittenBySuspendable();
            }
            m_processDomains.clear();
            m_writtenBySuspendable.clear();
            m_inProcess = false;
            m_gatherVars = false;
        }
        virtual void visit(AstFork* nodep) override {
            VL_RESTORER(m_gatherVars);
            if (m_inProcess) m_gatherVars = true;
            // If not in a process, we don't need to gather variables or domains
            iterateChildren(nodep);
        }
        virtual void visit(AstClass* nodep) override {
            UASSERT_OBJ(!m_classp, nodep, "Class under class");
            m_classp = nodep;
            iterateChildren(nodep);
            if (AstVarScope* const schedulerp = VN_AS(nodep->user1p(), VarScope)) {
                FileLine* const flp = nodep->fileline();
                auto* const symsp = new AstText{flp, "vlSymsp"};
                m_classSenItemsp = AstNode::addNext(
                    m_classSenItemsp,
                    new AstSenItem{flp, VEdgeType::ET_TRUE,
                                   makeCMethodHardBit(flp, schedulerp, "evalTriggers", symsp)});
                m_classResumep = AstNode::addNext(
                    m_classResumep,
                    makeCMethodHardVoid(flp, schedulerp, "resume", symsp->cloneTree(false))),
                m_classCommitp = AstNode::addNext(
                    m_classCommitp,
                    makeCMethodHardVoid(flp, schedulerp, "commit", symsp->cloneTree(false)));
                m_classUpdatep = AstNode::addNext(
                    m_classUpdatep, makeCMethodHardVoid(flp, schedulerp, "postTriggerUpdates",
                                                        symsp->cloneTree(false)));
            }
            m_classp = nullptr;
        }
        virtual void visit(AstScope* nodep) override {
            VL_RESTORER(m_scopep);
            m_scopep = nodep;
            iterateChildren(nodep);
            if (m_classp && m_classp->user1p()) {
                auto* newvscp = nodep->createTemp("__Vawait_count", 32);
                newvscp->varp()->combineType(VVarType::MEMBER);
            }
        }
        virtual void visit(AstCAwait* nodep) override {
            if (AstSenTree* const sensesp = nodep->sensesp()) {
                if (!sensesp->user1SetOnce()) createResumeActive(nodep);
                nodep->clearSensesp();  // Clear as these sentrees will get deleted later
                if (m_inProcess) m_processDomains.insert(sensesp);
                auto* const methodp = VN_AS(nodep->exprp(), CMethodHard);
                AstVarScope* const schedulerp = VN_AS(methodp->fromp(), VarRef)->varScopep();
                if (VN_AS(schedulerp->dtypep(), CDType)->isTriggerScheduler() && m_classp) {
                    FileLine* const flp = nodep->fileline();
                    AstVarScope* const schedulerp = getCreateClassTriggerScheduler();
                    auto* const thisp = new AstText{flp, "this"};
                    nodep->addHereThisAsNext(
                        makeCMethodHardVoid(flp, schedulerp, "reportAwait", thisp));
                    nodep->addNextHere(makeCMethodHardVoid(flp, schedulerp, "doneAwait",
                                                           thisp->cloneTree(false)));
                }
            }
        }
        virtual void visit(AstNodeVarRef* nodep) override {
            if (m_gatherVars && nodep->access().isWriteOrRW()
                && !nodep->varScopep()->user2SetOnce()) {
                m_writtenBySuspendable.push_back(nodep->varScopep());
            }
        }

        //--------------------
        virtual void visit(AstNodeMath*) override {}  // Accelerate
        virtual void visit(AstNode* nodep) override { iterateChildren(nodep); }

    public:
        // CONSTRUCTORS
        explicit AwaitVisitor(AstNetlist* nodep, LogicByScope& lbs, LogicByScope& classLbs,
                              AstCMethodHard*& classCommitp, AstCMethodHard*& classUpdatep,
                              std::map<const AstVarScope*, std::set<AstSenTree*>>& externalDomains)
            : m_netlistp{nodep}
            , m_scopeTopp{nodep->topScopep()->scopep()}
            , m_lbs{lbs}
            , m_classLbs{classLbs}
            , m_classCommitp{classCommitp}
            , m_classUpdatep{classUpdatep}
            , m_externalDomains{externalDomains} {
            iterate(nodep);
        }
        virtual ~AwaitVisitor() override = default;
    };
    LogicByScope lbs;
    LogicByScope classLbs;
    AstCMethodHard* classCommitp = nullptr;
    AstCMethodHard* classUpdatep = nullptr;
    std::map<const AstVarScope*, std::set<AstSenTree*>> externalDomains;
    AwaitVisitor{netlistp, lbs, classLbs, classCommitp, classUpdatep, externalDomains};
    return {std::move(lbs), std::move(classLbs), classCommitp, classUpdatep,
            std::move(externalDomains)};
}

//============================================================================
// Visits all forks and transforms their sub-statements into separate functions.

void transformForks(AstNetlist* const netlistp) {
    if (!v3Global.usesTiming()) return;
    // Transform all forked processes into functions
    class ForkVisitor final : public VNVisitor {
    private:
        // NODE STATE
        //  AstVar::user1()  -> bool.  Set true if the variable was declared before the current
        //                             fork.
        const VNUser1InUse m_inuser1;

        // STATE
        bool m_inClass = false;  // Are we in a class?
        AstFork* m_forkp = nullptr;  // Current fork
        AstCFunc* m_funcp = nullptr;  // Current function

        // METHODS
        // Remap local vars referenced by the given fork function
        // TODO: We should only pass variables to the fork that are
        // live in the fork body, but for that we need a proper data
        // flow analysis framework which we don't have at the moment
        void remapLocals(AstCFunc* const funcp, AstCCall* const callp) {
            const VNUser2InUse user2InUse;  // AstVarScope -> AstVarScope: var to remap to
            funcp->foreach<AstNodeVarRef>([&](AstNodeVarRef* refp) {
                AstVar* const varp = refp->varp();
                AstCDType* const dtypep = VN_CAST(varp->dtypep(), CDType);
                // If it a fork sync or an intra-assignment variable, pass it by value
                const bool passByValue = (dtypep && dtypep->isForkSync())
                                         || VString::startsWith(varp->name(), "__Vintra");
                // Only handle vars passed by value or locals declared before the fork
                if (!passByValue && (!varp->user1() || !varp->isFuncLocal())) return;
                if (passByValue) {
                    // We can just pass it to the new function
                } else if (m_forkp->joinType().join()) {
                    // If it's fork..join, we can refer to variables from the parent process
                    if (m_funcp->user1SetOnce()) {  // Only do this once per function
                        // Move all locals to the heap before the fork
                        auto* const awaitp = new AstCAwait{
                            m_forkp->fileline(), new AstCStmt{m_forkp->fileline(), "VlNow{}"}};
                        awaitp->statement(true);
                        m_forkp->addHereThisAsNext(awaitp);
                    }
                } else {
                    refp->v3warn(E_UNSUPPORTED, "Unsupported: variable local to a forking process "
                                                "accessed in a fork..join_any or fork..join_none");
                    return;
                }
                // Remap the reference
                AstVarScope* const vscp = refp->varScopep();
                if (!vscp->user2p()) {
                    // Clone the var to the new function
                    AstVar* const varp = refp->varp();
                    AstVar* const newvarp
                        = new AstVar{varp->fileline(), VVarType::BLOCKTEMP, varp->name(), varp};
                    newvarp->funcLocal(true);
                    newvarp->direction(passByValue ? VDirection::INPUT : VDirection::REF);
                    funcp->addArgsp(newvarp);
                    AstVarScope* const newvscp
                        = new AstVarScope{newvarp->fileline(), funcp->scopep(), newvarp};
                    funcp->scopep()->addVarp(newvscp);
                    vscp->user2p(newvscp);
                    callp->addArgsp(new AstVarRef{refp->fileline(), vscp, VAccess::READ});
                }
                auto* const newvscp = VN_AS(vscp->user2p(), VarScope);
                refp->varScopep(newvscp);
                refp->varp(newvscp->varp());
            });
        }

        // VISITORS
        virtual void visit(AstNodeModule* nodep) override {
            VL_RESTORER(m_inClass);
            m_inClass = VN_IS(nodep, Class);
            iterateChildren(nodep);
        }
        virtual void visit(AstCFunc* nodep) override {
            m_funcp = nodep;
            iterateChildren(nodep);
            m_funcp = nullptr;
        }
        virtual void visit(AstVar* nodep) override { nodep->user1(true); }
        virtual void visit(AstFork* nodep) override {
            VL_RESTORER(m_forkp);
            m_forkp = nodep;
            iterateChildrenConst(nodep);  // Const, so we don't iterate the calls twice
            // Replace self with the function calls (no co_await, as we don't want the main
            // process to suspend whenever any of the children do)
            nodep->replaceWith(nodep->stmtsp()->unlinkFrBackWithNext());
            VL_DO_DANGLING(nodep->deleteTree(), nodep);
        }
        virtual void visit(AstBegin* nodep) override {
            UASSERT_OBJ(m_forkp, nodep, "Begin outside of a fork");
            UASSERT_OBJ(!nodep->name().empty(), nodep, "Begin needs a name");
            FileLine* const flp = nodep->fileline();
            // Create a function to put this begin's statements in
            AstCFunc* const newfuncp
                = new AstCFunc{flp, nodep->name(), m_funcp->scopep(), "VlCoroutine"};
            m_funcp->addNextHere(newfuncp);
            newfuncp->isLoose(m_funcp->isLoose());
            newfuncp->slow(m_funcp->slow());
            newfuncp->isConst(m_funcp->isConst());
            newfuncp->declPrivate(true);
            // Replace the begin with a call to the newly created function
            auto* const callp = new AstCCall{flp, newfuncp};
            nodep->replaceWith(callp);
            // If we're in a class, add a vlSymsp arg
            if (m_inClass) {
                newfuncp->argTypes(EmitCBaseVisitor::symClassVar());
                callp->argTypes("vlSymsp");
            }
            // Put the begin's statements in the function, delete the begin
            newfuncp->addStmtsp(nodep->stmtsp()->unlinkFrBackWithNext());
            VL_DO_DANGLING(nodep->deleteTree(), nodep);
            remapLocals(newfuncp, callp);
        }

        //--------------------
        virtual void visit(AstNodeMath*) override {}  // Accelerate
        virtual void visit(AstNode* nodep) override { iterateChildren(nodep); }

    public:
        // CONSTRUCTORS
        explicit ForkVisitor(AstNetlist* nodep) { iterate(nodep); }
        virtual ~ForkVisitor() override = default;
    };
    ForkVisitor{netlistp};
    V3Global::dumpCheckGlobalTree("sched_forks", 0, v3Global.opt.dumpTreeLevel(__FILE__) >= 6);
}

}  // namespace V3Sched
