// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Prepare AST for dynamic scheduler features
//
// Code available from: https://verilator.org
//
//*************************************************************************
//
// Copyright 2003-2021 by Wilson Snyder. This program is free software; you
// can redistribute it and/or modify it under the terms of either the GNU
// Lesser General Public License Version 3 or the Perl Artistic License
// Version 2.0.
// SPDX-License-Identifier: LGPL-3.0-only OR Artistic-2.0
//
//*************************************************************************
// V3DynamicScheduler's Transformations:
//
//      Each Delay, TimingControl, Wait:
//          Mark containing task for dynamic scheduling
//          Mark containing process for dynamic scheduling
//      Each Task:
//          If it's virtual, mark it
//      Each task calling a marked task:
//          Mark it for dynamic scheduling
//      Each process calling a marked task:
//          Mark it for dynamic scheduling
//      Each variable written to in a marked process/task:
//          Mark it for dynamic scheduling
//
//      Each timing control waiting on an unmarked variable:
//          If waiting on ANYEDGE on more than 1-bit wide signals:
//              Change edge type to BOTHEDGE
//
//      Each marked process:
//          Wrap its statements into begin...end so it won't get split
//
//      Each process:
//          If waiting on ANYEDGE on a marked variable:
//              Transform process into function with a body like this:
//                  forever begin
//                      @(sensp);
//                      fork process; join_none
//                  end
//
//      Each AssignDly:
//          If in marked process:
//              Transform into:
//                  fork @__VdlyEvent__ lhsp = rhsp; join_none
//
//      Each Fork:
//          Move each statement to a new function
//          Add call to new function in place of moved statement
//
//      Each TimingControl, Wait:
//          Create event variables for triggering those.
//          For Wait:
//              Transform into:
//                  while (!wait.condp)
//                      @(vars from wait.condp);
//                  wait.bodysp;
//              (process the new TimingControl as specified below)
//          For TimingControl:
//              If waiting on event, leave it as is
//              If waiting on posedge:
//                  Create a posedge event variable for the awaited signal
//              If waiting on negedge:
//                  Create a negedge event variable for the awaited signal
//              If waiting on bothedge:
//                  Split it into posedge and negedge
//                  Create a posedge event variable for the awaited signal
//                  Create a negedge event variable for the awaited signal
//              If waiting on anyedge:
//                  Create a anyedge event variable for the awaited signal
//              For each variable in the condition being waited on:
//                  Create an anyedge event variable for the awaited variable
//
//      Each assignment:
//          If there is an edge event variable associated with the LHS:
//              Create an EventTrigger for this event variable under an if that checks if the edge
//              occurs
//      Each clocked var:
//          If there is an edge event variable associated with it:
//              Create a new Active for this edge with an EventTrigger for this event variable
//
//*************************************************************************

#include "config_build.h"
#include "verilatedos.h"

#include "V3Global.h"
#include "V3DynamicScheduler.h"
#include "V3Ast.h"
#include "V3EmitCBase.h"

static AstVarScope* getCreateEvent(AstVarScope* vscp, VEdgeType edgeType) {
    UASSERT_OBJ(vscp->scopep(), vscp, "Var unscoped");
    if (auto* eventp = vscp->varp()->edgeEvent(edgeType)) return eventp;
    string newvarname = (string("__VedgeEvent__") + vscp->scopep()->nameDotless() + "__"
                         + edgeType.ascii() + "__" + vscp->varp()->name());
    auto* newvarp = new AstVar{vscp->fileline(), AstVarType::VAR, newvarname,
                               vscp->findBasicDType(AstBasicDTypeKwd::EVENTVALUE)};
    newvarp->sigPublic(true);
    vscp->scopep()->modp()->addStmtp(newvarp);
    auto* newvscp = new AstVarScope{vscp->fileline(), vscp->scopep(), newvarp};
    vscp->user1p(newvscp);
    vscp->scopep()->addVarp(newvscp);
    vscp->varp()->edgeEvent(edgeType, newvscp);
    return newvscp;
}

//######################################################################

class DynamicSchedulerMarkDynamicVisitor final : public AstNVisitor {
private:
    // NODE STATE
    //  AstNode::user1()      -> bool.  Set true if process/function uses constructs like delays,
    //                                  timing controls, waits, forks
    AstUser1InUse m_inuser1;

    // STATE
    AstNode* m_proc = nullptr;
    bool repeat = false;

    // METHODS
    VL_DEBUG_FUNC;  // Declare debug()

    // VISITORS
    virtual void visit(AstNodeProcedure* nodep) override {
        VL_RESTORER(m_proc);
        {
            m_proc = nodep;
            if (!nodep->user1()) { iterateChildren(nodep); }
        }
    }
    virtual void visit(AstNodeFTask* nodep) override {
        VL_RESTORER(m_proc);
        {
            m_proc = nodep;
            if (!nodep->user1()) {
                if (nodep->isVirtual())
                    nodep->user1(true);
                else
                    iterateChildren(nodep);
                if (nodep->user1()) repeat = true;
            }
        }
    }
    virtual void visit(AstCFunc* nodep) override {
        VL_RESTORER(m_proc);
        {
            m_proc = nodep;
            if (!nodep->user1()) {
                if (nodep->isVirtual())
                    nodep->user1(true);
                else
                    iterateChildren(nodep);
                if (nodep->user1()) repeat = true;
            }
        }
    }
    virtual void visit(AstDelay* nodep) override {
        if (m_proc) m_proc->user1(true);
    }
    virtual void visit(AstNodeAssign* nodep) override {
        if (nodep->delayp() && m_proc) m_proc->user1(true);
    }
    virtual void visit(AstTimingControl* nodep) override {
        if (m_proc) m_proc->user1(true);
    }
    virtual void visit(AstWait* nodep) override {
        if (m_proc) m_proc->user1(true);
    }
    virtual void visit(AstFork* nodep) override {
        if (m_proc) m_proc->user1(m_proc->user1() || !nodep->joinType().joinNone());
        iterateChildren(nodep);
    }
    virtual void visit(AstNodeFTaskRef* nodep) override {
        if (m_proc && nodep->taskp()->user1()) m_proc->user1(true);
    }
    virtual void visit(AstMethodCall* nodep) override {
        if (m_proc && nodep->taskp()->user1()) m_proc->user1(true);
    }
    virtual void visit(AstCMethodCall* nodep) override {
        if (m_proc && nodep->funcp()->user1()) m_proc->user1(true);
    }

    //--------------------
    virtual void visit(AstNode* nodep) override { iterateChildren(nodep); }

public:
    // CONSTRUCTORS
    explicit DynamicSchedulerMarkDynamicVisitor(AstNetlist* nodep) {
        do {
            repeat = false;
            iterate(nodep);
        } while (repeat);
    }
    virtual ~DynamicSchedulerMarkDynamicVisitor() override {}
};

//######################################################################

class DynamicSchedulerMarkVariablesVisitor final : public AstNVisitor {
private:
    // NODE STATE
    //  AstNode::user1()      -> bool.  Set true if process/function uses constructs like delays,
    //                                  timing controls, waits, forks
    // AstUser1InUse    m_inuser1;      (Allocated for use in DynamicSchedulerMarkDynamicVisitor)

    // STATE
    bool m_dynamic = false;

    // METHODS
    VL_DEBUG_FUNC;  // Declare debug()

    // VISITORS
    virtual void visit(AstNodeProcedure* nodep) override {
        VL_RESTORER(m_dynamic);
        m_dynamic = nodep->user1();
        iterateChildren(nodep);
    }
    virtual void visit(AstNodeFTask* nodep) override {
        VL_RESTORER(m_dynamic);
        m_dynamic = nodep->user1();
        iterateChildren(nodep);
    }
    virtual void visit(AstCFunc* nodep) override {
        VL_RESTORER(m_dynamic);
        m_dynamic = nodep->user1();
        iterateChildren(nodep);
    }
    virtual void visit(AstFork* nodep) override {
        VL_RESTORER(m_dynamic);
        m_dynamic = true;
        iterateChildren(nodep);
    }
    virtual void visit(AstVarRef* nodep) override {
        if (m_dynamic && nodep->access().isWriteOrRW()) { nodep->varp()->isDynamic(true); }
    }

    //--------------------
    virtual void visit(AstNode* nodep) override { iterateChildren(nodep); }

public:
    // CONSTRUCTORS
    explicit DynamicSchedulerMarkVariablesVisitor(AstNetlist* nodep) { iterate(nodep); }
    virtual ~DynamicSchedulerMarkVariablesVisitor() override {}
};

//######################################################################

class DynamicSchedulerAnyedgeVisitor final : public AstNVisitor {
private:
    // NODE STATE
    //  AstNode::user1()      -> bool.  Set true if process/function uses constructs like delays,
    //                                  timing controls, waits, forks
    // AstUser1InUse    m_inuser1;      (Allocated for use in DynamicSchedulerMarkDynamicVisitor)

    // METHODS
    VL_DEBUG_FUNC;  // Declare debug()
    bool onlySenItemInSenTree(AstSenItem* nodep) {
        // Only one if it's not in a list
        return (!nodep->nextp() && nodep->backp()->nextp() != nodep);
    }

    // VISITORS
    virtual void visit(AstSenItem* nodep) override {
        if (!onlySenItemInSenTree(nodep))
            return;
        else if (nodep->varrefp() && !nodep->varrefp()->varp()->isDynamic()
                 && !VN_IS(nodep->varrefp()->dtypep(), NodeArrayDType)) {
            if (const AstBasicDType* const basicp = nodep->varrefp()->dtypep()->basicp()) {
                if (!basicp->isEventValue() && nodep->edgeType() == VEdgeType::ET_ANYEDGE
                    && nodep->varrefp()->width1()) {
                    nodep->edgeType(VEdgeType::ET_BOTHEDGE);
                }
            }
        }
    }

    //--------------------
    virtual void visit(AstNode* nodep) override { iterateChildren(nodep); }

public:
    // CONSTRUCTORS
    explicit DynamicSchedulerAnyedgeVisitor(AstNetlist* nodep) { iterate(nodep); }
    virtual ~DynamicSchedulerAnyedgeVisitor() override {}
};

//######################################################################

class DynamicSchedulerWrapProcessVisitor final : public AstNVisitor {
private:
    // NODE STATE
    //  AstNodeProcedure::user1()      -> bool.  Set true if 'dynamic' (shouldn't be split up)
    // AstUser1InUse    m_inuser1;      (Allocated for use in DynamicSchedulerMarkDynamicVisitor)

    // METHODS
    VL_DEBUG_FUNC;  // Declare debug()

    // VISITORS
    virtual void visit(AstNodeProcedure* nodep) override {
        if (nodep->user1()) {
            // Prevent splitting by wrapping body in an AstBegin
            auto* bodysp = nodep->bodysp()->unlinkFrBackWithNext();
            nodep->addStmtp(new AstBegin{nodep->fileline(), "", bodysp});
        }
    }

    //--------------------
    virtual void visit(AstNode* nodep) override { iterateChildren(nodep); }

public:
    // CONSTRUCTORS
    explicit DynamicSchedulerWrapProcessVisitor(AstNetlist* nodep) { iterate(nodep); }
    virtual ~DynamicSchedulerWrapProcessVisitor() override {}
};

//######################################################################

class DynamicSchedulerIntraAssignDelayVisitor final : public AstNVisitor {
private:
    // STATE
    using VarMap = std::map<const std::pair<AstNodeModule*, string>, AstVar*>;
    VarMap m_modVarMap;  // Table of new var names created under module
    size_t m_count = 0;

    // METHODS
    AstVarScope* getCreateVar(AstVarScope* oldvarscp, const string& name) {
        UASSERT_OBJ(oldvarscp->scopep(), oldvarscp, "Var unscoped");
        AstVar* varp;
        AstNodeModule* addmodp = oldvarscp->scopep()->modp();
        // We need a new AstVar, but only one for all scopes, to match the new AstVarScope
        const auto it = m_modVarMap.find(make_pair(addmodp, name));
        if (it != m_modVarMap.end()) {
            // Created module's AstVar earlier under some other scope
            varp = it->second;
        } else {
            varp = new AstVar{oldvarscp->fileline(), AstVarType::BLOCKTEMP, name,
                              oldvarscp->varp()};
            varp->dtypeFrom(oldvarscp);
            addmodp->addStmtp(varp);
            m_modVarMap.emplace(make_pair(addmodp, name), varp);
        }
        AstVarScope* varscp = new AstVarScope{oldvarscp->fileline(), oldvarscp->scopep(), varp};
        oldvarscp->scopep()->addVarp(varscp);
        return varscp;
    }
    VL_DEBUG_FUNC;  // Declare debug()

    // VISITORS
    virtual void visit(AstAssign* nodep) override {
        if (auto* delayp = nodep->delayp()) {
            delayp->unlinkFrBack();
            auto* lhsp = VN_CAST(nodep->lhsp(), VarRef);
            auto* newvscp
                = getCreateVar(lhsp->varScopep(),
                               "__Vintraval" + std::to_string(m_count++) + "__" + lhsp->name());
            nodep->addHereThisAsNext(new AstAssign{
                nodep->fileline(), new AstVarRef{nodep->fileline(), newvscp, VAccess::WRITE},
                nodep->rhsp()->unlinkFrBack()});
            nodep->rhsp(new AstVarRef{nodep->fileline(), newvscp, VAccess::READ});
            nodep->addHereThisAsNext(new AstDelay{delayp->fileline(), delayp, nullptr});
        }
    }
    virtual void visit(AstAssignDly* nodep) override {
        if (auto* delayp = nodep->delayp()) {
            auto* lhsp = VN_CAST(nodep->lhsp(), VarRef);
            auto* newvscp
                = getCreateVar(lhsp->varScopep(),
                               "__Vintraval" + std::to_string(m_count++) + "__" + lhsp->name());
            nodep->addHereThisAsNext(new AstAssign{
                nodep->fileline(), new AstVarRef{nodep->fileline(), newvscp, VAccess::WRITE},
                nodep->rhsp()->unlinkFrBack()});
            nodep->rhsp(new AstVarRef{nodep->fileline(), newvscp, VAccess::READ});
            delayp->unlinkFrBack();
            auto* forkp = new AstFork{nodep->fileline(), "", nullptr};
            forkp->joinType(VJoinType::JOIN_NONE);
            nodep->replaceWith(forkp);
            forkp->addStmtsp(new AstDelay{delayp->fileline(), delayp, nodep});
        }
    }

    //--------------------
    virtual void visit(AstNode* nodep) override { iterateChildren(nodep); }

public:
    // CONSTRUCTORS
    explicit DynamicSchedulerIntraAssignDelayVisitor(AstNetlist* nodep) { iterate(nodep); }
    virtual ~DynamicSchedulerIntraAssignDelayVisitor() override {}
};

//######################################################################

class DynamicSchedulerAlwaysVisitor final : public AstNVisitor {
private:
    // NODE STATE
    //  AstNode::user1()      -> bool.  Set true if process/function uses constructs like delays,
    //                                  timing controls, waits, forks
    // AstUser1InUse    m_inuser1;      (Allocated for use in DynamicSchedulerMarkDynamicVisitor)

    // STATE
    AstScope* m_scopep = nullptr;
    size_t m_count = 0;

    // METHODS
    VL_DEBUG_FUNC;  // Declare debug()

    // VISITORS
    virtual void visit(AstScope* nodep) override {
        m_scopep = nodep;
        iterateChildren(nodep);
        m_scopep = nullptr;
    }
    virtual void visit(AstTopScope* nodep) override {
        iterateChildren(nodep);
        auto* activep = new AstAlwaysDelayed{
            nodep->fileline(),
            new AstCStmt{nodep->fileline(),
                         "vlSymsp->__Vm_eventDispatcher.resumeAllTriggered();\n"}};
        nodep->scopep()->addActivep(activep);
    }
    virtual void visit(AstAlways* nodep) override {
        auto* sensesp = nodep->sensesp();
        if (sensesp && sensesp->sensesp()
            && sensesp->sensesp()->edgeType() == VEdgeType::ET_ANYEDGE
            && sensesp->sensesp()->varrefp() && sensesp->sensesp()->varrefp()->varp()->isDynamic()
            && nodep->bodysp()) {
            auto* eventp = getCreateEvent(sensesp->sensesp()->varrefp()->varScopep(),
                                          VEdgeType::ET_ANYEDGE);

            auto fl = m_scopep->fileline();

            auto newFuncName = "_anyedge_" + std::to_string(m_count++);
            auto* newFuncpr = new AstCFunc{fl, newFuncName, m_scopep, "CoroutineTask"};
            newFuncpr->isStatic(false);
            newFuncpr->isLoose(true);
            m_scopep->addActivep(newFuncpr);

            auto* beginp = new AstBegin{fl, "", nodep->bodysp()->unlinkFrBackWithNext()};
            auto* forkp = new AstFork{fl, "", beginp};
            forkp->joinType(VJoinType::JOIN_NONE);
            auto* whilep
                = new AstWhile{fl, new AstConst{fl, AstConst::BitTrue()},
                               new AstTimingControl{fl, sensesp->cloneTree(false), forkp}};
            newFuncpr->addStmtsp(whilep);
            AstCCall* const callp = new AstCCall{fl, newFuncpr};
            auto* initialp = new AstInitial{fl, callp};
            m_scopep->addActivep(initialp);
            nodep->unlinkFrBack();
            VL_DO_DANGLING(nodep->deleteTree(), nodep);
            newFuncpr->user1(true);
        }
    }

    //--------------------
    virtual void visit(AstNode* nodep) override { iterateChildren(nodep); }

public:
    // CONSTRUCTORS
    explicit DynamicSchedulerAlwaysVisitor(AstNetlist* nodep) { iterate(nodep); }
    virtual ~DynamicSchedulerAlwaysVisitor() override {}
};

//######################################################################

class DynamicSchedulerForkVisitor final : public AstNVisitor {
private:
    // NODE STATE
    //  AstNode::user1()      -> bool.  Set true if process/function uses constructs like delays,
    //                                  timing controls, waits, forks
    // AstUser1InUse    m_inuser1;      (Allocated for use in DynamicSchedulerMarkDynamicVisitor)

    // STATE
    AstScope* m_scopep = nullptr;
    std::map<AstVarScope*, AstVarScope*> m_locals;
    size_t m_count = 0;
    AstVar* m_joinEventp;
    AstVar* m_joinCounterp;
    AstClassRefDType* m_joinDTypep;
    AstCFunc* m_joinNewp;

    enum { FORK, GATHER, REPLACE } m_mode = FORK;

    // METHODS
    VL_DEBUG_FUNC;  // Declare debug()

    // VISITORS
    virtual void visit(AstScope* nodep) override {
        m_scopep = nodep;
        iterateChildren(nodep);
        m_scopep = nullptr;
    }
    virtual void visit(AstVarRef* nodep) override {
        if (m_mode == GATHER) {
            if (nodep->varp()->varType() == AstVarType::BLOCKTEMP)
                m_locals.insert(std::make_pair(nodep->varScopep(), nullptr));
        } else if (m_mode == REPLACE) {
            if (auto* newvscp = m_locals[nodep->varScopep()]) {
                nodep->varScopep(newvscp);
                nodep->varp(newvscp->varp());
            }
        }
    }
    virtual void visit(AstFork* nodep) override {
        if (m_mode != FORK) {
            iterateChildren(nodep);
            return;
        }
        if (nodep->user2SetOnce()) return;

        AstVarScope* joinVscp = nullptr;
        if (!nodep->joinType().joinNone()) {
            auto* joinVarp
                = new AstVar{nodep->fileline(), AstVarType::BLOCKTEMP,
                             "__Vfork__" + std::to_string(m_count) + "__join", m_joinDTypep};
            joinVarp->funcLocal(true);
            joinVscp = new AstVarScope{joinVarp->fileline(), m_scopep, joinVarp};
            m_scopep->addVarp(joinVscp);
            nodep->addHereThisAsNext(joinVarp);
        }

        VL_RESTORER(m_mode);
        auto stmtp = nodep->stmtsp();
        size_t procCount = 0;
        while (stmtp) {
            m_locals.clear();
            m_mode = GATHER;
            iterateChildren(stmtp);
            if (joinVscp) m_locals.insert(std::make_pair(joinVscp, nullptr));

            AstCFunc* const cfuncp = new AstCFunc{stmtp->fileline(),
                                                  "__Vfork__" + std::to_string(m_count++) + "__"
                                                      + std::to_string(procCount++),
                                                  m_scopep, "CoroutineTask"};
            m_scopep->addActivep(cfuncp);
            cfuncp->user1(true);

            // Create list of arguments and move to function
            AstNode* argsp = nullptr;
            for (auto& p : m_locals) {
                auto* varscp = p.first;
                auto* varp = varscp->varp()->cloneTree(false);
                varp->funcLocal(true);
                varp->direction(VDirection::INPUT);
                cfuncp->addArgsp(varp);
                AstVarScope* const newvscp = new AstVarScope{varp->fileline(), m_scopep, varp};
                m_scopep->addVarp(newvscp);
                p.second = newvscp;
                argsp = AstNode::addNext(argsp,
                                         new AstVarRef{stmtp->fileline(), varscp, VAccess::READ});
            }
            auto* ccallp = new AstCCall{stmtp->fileline(), cfuncp, argsp};
            stmtp->replaceWith(ccallp);

            if (joinVscp) {
                auto* counterSelp = new AstMemberSel{
                    nodep->fileline(), new AstVarRef{nodep->fileline(), joinVscp, VAccess::WRITE},
                    m_joinCounterp->dtypep()};
                counterSelp->varp(m_joinCounterp);
                stmtp->addNext(
                    new AstAssign{nodep->fileline(), counterSelp,
                                  new AstSub{nodep->fileline(), counterSelp->cloneTree(false),
                                             new AstConst{nodep->fileline(), 1}}});
                auto* eventSelp = new AstMemberSel{
                    nodep->fileline(), new AstVarRef{nodep->fileline(), joinVscp, VAccess::WRITE},
                    m_joinEventp->dtypep()};
                eventSelp->varp(m_joinEventp);
                stmtp->addNext(new AstEventTrigger{nodep->fileline(), eventSelp});
            }

            cfuncp->addStmtsp(stmtp);
            m_mode = REPLACE;
            iterateChildren(cfuncp);
            stmtp = ccallp->nextp();
        }

        if (joinVscp) {
            auto* cnewp = new AstCNew{nodep->fileline(), m_joinNewp, nullptr};
            cnewp->dtypep(m_joinDTypep);
            auto* assignp
                = new AstAssign{nodep->fileline(),
                                new AstVarRef{nodep->fileline(), joinVscp, VAccess::WRITE}, cnewp};
            nodep->addHereThisAsNext(assignp);

            auto* counterSelp = new AstMemberSel{
                nodep->fileline(), new AstVarRef{nodep->fileline(), joinVscp, VAccess::WRITE},
                m_joinCounterp->dtypep()};
            counterSelp->varp(m_joinCounterp);
            assignp = new AstAssign{
                nodep->fileline(), counterSelp,
                new AstConst{nodep->fileline(), nodep->joinType().joinAny() ? 1 : procCount}};
            nodep->addHereThisAsNext(assignp);

            counterSelp = new AstMemberSel{
                nodep->fileline(), new AstVarRef{nodep->fileline(), joinVscp, VAccess::READ},
                m_joinCounterp->dtypep()};
            counterSelp->varp(m_joinCounterp);
            auto* eventSelp = new AstMemberSel{
                nodep->fileline(), new AstVarRef{nodep->fileline(), joinVscp, VAccess::READ},
                m_joinEventp->dtypep()};
            eventSelp->varp(m_joinEventp);
            nodep->addNextHere(new AstWhile{
                nodep->fileline(),
                new AstGt{nodep->fileline(), counterSelp, new AstConst{nodep->fileline(), 0}},
                new AstTimingControl{
                    nodep->fileline(),
                    new AstSenTree{
                        nodep->fileline(),
                        new AstSenItem{nodep->fileline(), VEdgeType::ET_ANYEDGE, eventSelp}},
                    nullptr}});
        }
    }

    //--------------------
    virtual void visit(AstNode* nodep) override { iterateChildren(nodep); }

public:
    // CONSTRUCTORS
    explicit DynamicSchedulerForkVisitor(AstNetlist* nodep) {
        auto* joinClassp = new AstClass{nodep->fileline(), "Join"};
        auto* joinClassPackagep = new AstClassPackage{nodep->fileline(), "Join__Vclpkg"};
        joinClassp->classOrPackagep(joinClassPackagep);
        joinClassPackagep->classp(joinClassp);
        nodep->addModulep(joinClassPackagep);
        nodep->addModulep(joinClassp);
        AstCell* const cellp = new AstCell{joinClassPackagep->fileline(),
                                           joinClassPackagep->fileline(),
                                           joinClassPackagep->name(),
                                           joinClassPackagep->name(),
                                           nullptr,
                                           nullptr,
                                           nullptr};
        cellp->modp(joinClassPackagep);
        v3Global.rootp()->topModulep()->addStmtp(cellp);
        auto* joinScopep = new AstScope{nodep->fileline(), joinClassp, "Join", nullptr, nullptr};
        joinClassp->addMembersp(joinScopep);
        m_joinEventp = new AstVar{nodep->fileline(), AstVarType::MEMBER, "wakeEvent",
                                  nodep->findBasicDType(AstBasicDTypeKwd::EVENTVALUE)};
        joinClassp->addMembersp(m_joinEventp);
        m_joinCounterp = new AstVar{nodep->fileline(), AstVarType::MEMBER, "counter",
                                    nodep->findSigned32DType()};
        joinClassp->addMembersp(m_joinCounterp);
        m_joinDTypep = new AstClassRefDType{nodep->fileline(), joinClassp, nullptr};
        m_joinDTypep->dtypep(m_joinDTypep);
        nodep->typeTablep()->addTypesp(m_joinDTypep);
        m_joinNewp = new AstCFunc{nodep->fileline(), "new", joinScopep, ""};
        m_joinNewp->argTypes(EmitCBaseVisitor::symClassVar());
        m_joinNewp->isConstructor(true);
        joinScopep->addActivep(m_joinNewp);
        iterate(nodep);
    }
    virtual ~DynamicSchedulerForkVisitor() override {}
};

//######################################################################

class DynamicSchedulerAssignDlyVisitor final : public AstNVisitor {
private:
    // NODE STATE
    //  AstNode::user1()      -> bool.  Set true if process/function uses constructs like delays,
    //                                  timing controls, waits, forks
    // AstUser1InUse    m_inuser1;      (Allocated for use in DynamicSchedulerMarkDynamicVisitor)

    // STATE
    AstVarScope* m_dlyEvent = nullptr;
    AstScope* m_scopep = nullptr;
    bool m_dynamic = false;
    bool m_inFork = false;

    // METHODS
    VL_DEBUG_FUNC;  // Declare debug()

    AstVarScope* getCreateDlyEvent() {
        if (m_dlyEvent) return m_dlyEvent;
        string newvarname = "__VdlyEvent__";
        auto fl = new FileLine{m_scopep->fileline()};
        fl->warnOff(V3ErrorCode::UNOPTFLAT, true);
        auto* newvarp = new AstVar{fl, AstVarType::MODULETEMP, newvarname,
                                   m_scopep->findBasicDType(AstBasicDTypeKwd::EVENTVALUE)};
        m_scopep->modp()->addStmtp(newvarp);
        auto* newvscp = new AstVarScope{fl, m_scopep, newvarp};
        m_scopep->addVarp(newvscp);
        auto* triggerp = new AstEventTrigger{fl, new AstVarRef{fl, newvscp, VAccess::WRITE}};
        auto* activep = new AstAlwaysDelayed{fl, triggerp};
        m_scopep->addActivep(activep);
        return m_dlyEvent = newvscp;
    }

    // VISITORS
    virtual void visit(AstScope* nodep) override {
        m_scopep = nodep;
        iterateChildren(nodep);
        m_scopep = nullptr;
    }
    virtual void visit(AstNodeProcedure* nodep) override {
        VL_RESTORER(m_dynamic);
        m_dynamic = nodep->user1();
        iterateChildren(nodep);
    }
    virtual void visit(AstNodeFTask* nodep) override {
        VL_RESTORER(m_dynamic);
        m_dynamic = nodep->user1();
        iterateChildren(nodep);
    }
    virtual void visit(AstCFunc* nodep) override {
        VL_RESTORER(m_dynamic);
        m_dynamic = nodep->user1();
        iterateChildren(nodep);
    }
    virtual void visit(AstFork* nodep) override {
        VL_RESTORER(m_dynamic);
        m_dynamic = true;
        VL_RESTORER(m_inFork);
        m_inFork = true;
        iterateChildren(nodep);
    }
    virtual void visit(AstBegin* nodep) override {
        VL_RESTORER(m_inFork);
        m_inFork = false;
        iterateChildren(nodep);
    }
    virtual void visit(AstAssignDly* nodep) override {
        if (!m_dynamic) return;
        auto fl = nodep->fileline();
        auto* eventp = getCreateDlyEvent();
        auto* assignp
            = new AstAssign{fl, nodep->lhsp()->unlinkFrBack(), nodep->rhsp()->unlinkFrBack()};
        auto* timingControlp = new AstTimingControl{
            fl,
            new AstSenTree{fl, new AstSenItem{fl, VEdgeType::ET_ANYEDGE,
                                              new AstVarRef{fl, eventp, VAccess::READ}}},
            assignp};
        if (m_inFork) {
            nodep->replaceWith(timingControlp);
        } else {
            auto* forkp = new AstFork{nodep->fileline(), "", timingControlp};
            forkp->joinType(VJoinType::JOIN_NONE);
            nodep->replaceWith(forkp);
        }
        VL_DO_DANGLING(delete nodep, nodep);
    }

    //--------------------
    virtual void visit(AstNode* nodep) override { iterateChildren(nodep); }

public:
    // CONSTRUCTORS
    explicit DynamicSchedulerAssignDlyVisitor(AstNetlist* nodep) { iterate(nodep); }
    virtual ~DynamicSchedulerAssignDlyVisitor() override {}
};

//######################################################################

class DynamicSchedulerCreateEventsVisitor final : public AstNVisitor {
private:
    // NODE STATE
    // AstVar::user1()      -> bool.  Set true if variable is waited on
    AstUser1InUse m_inuser1;

    // STATE
    using VarScopeSet = std::set<AstVarScope*>;
    VarScopeSet m_waitVars;

    bool m_inTimingControlSens = false;
    bool m_inWait = false;

    // METHODS
    VL_DEBUG_FUNC;  // Declare debug()

    // VISITORS
    virtual void visit(AstTimingControl* nodep) override {
        VL_RESTORER(m_inTimingControlSens);
        m_inTimingControlSens = true;
        iterateAndNextNull(nodep->sensesp());
        m_inTimingControlSens = false;
        iterateAndNextNull(nodep->stmtsp());
    }
    virtual void visit(AstWait* nodep) override {
        VL_RESTORER(m_inWait);
        m_inWait = true;
        iterateAndNextNull(nodep->condp());
        if (m_waitVars.empty()) {
            if (nodep->bodysp())
                nodep->replaceWith(nodep->bodysp()->unlinkFrBackWithNext());
            else
                nodep->unlinkFrBack();
        } else {
            auto fl = nodep->fileline();
            AstNode* senitemsp = nullptr;
            for (auto* vscp : m_waitVars) {
                AstVarScope* eventp = vscp->varp()->dtypep()->basicp()->isEventValue()
                                          ? vscp
                                          : getCreateEvent(vscp, VEdgeType::ET_ANYEDGE);
                senitemsp = AstNode::addNext(
                    senitemsp, new AstSenItem{fl, VEdgeType::ET_ANYEDGE,
                                              new AstVarRef{fl, eventp, VAccess::READ}});
            }
            auto* condp = nodep->condp()->unlinkFrBack();
            auto* timingControlp = new AstTimingControl{
                fl, new AstSenTree{fl, VN_CAST(senitemsp, SenItem)}, nullptr};
            auto* whilep = new AstWhile{fl, new AstLogNot{fl, condp}, timingControlp};
            if (nodep->bodysp()) whilep->addNext(nodep->bodysp()->unlinkFrBackWithNext());
            nodep->replaceWith(whilep);
            m_waitVars.clear();
        }
        VL_DO_DANGLING(nodep->deleteTree(), nodep);
    }
    virtual void visit(AstSenItem* nodep) override {
        if (m_inTimingControlSens) {
            if (nodep->edgeType() == VEdgeType::ET_BOTHEDGE) {
                nodep->addNextHere(nodep->cloneTree(false));
                nodep->edgeType(VEdgeType::ET_POSEDGE);
                VN_CAST(nodep->nextp(), SenItem)->edgeType(VEdgeType::ET_NEGEDGE);
            }
        }
        iterateChildren(nodep);
    }
    virtual void visit(AstVarRef* nodep) override {
        if (m_inWait) {
            nodep->varp()->user1(true);
            m_waitVars.insert(nodep->varScopep());
        } else if (m_inTimingControlSens) {
            if (!nodep->varp()->dtypep()->basicp()->isEventValue()) {
                nodep->varp()->user1(true);
                auto edgeType = VN_CAST(nodep->backp(), SenItem)->edgeType();
                nodep->varScopep(getCreateEvent(nodep->varScopep(), edgeType));
                nodep->varp(nodep->varScopep()->varp());
            }
        }
    }
    virtual void visit(AstMemberSel* nodep) override {
        if (m_inWait) {
            nodep->varp()->user1(true);
            m_waitVars.insert(VN_CAST(nodep->fromp(), VarRef)->varScopep());
        } else if (m_inTimingControlSens) {
            if (!nodep->varp()->dtypep()->basicp()->isEventValue()) {
                nodep->varp()->user1(true);
                auto edgeType = VN_CAST(nodep->backp(), SenItem)->edgeType();
                nodep->replaceWith(new AstVarRef(
                    nodep->fileline(),
                    getCreateEvent(VN_CAST(nodep->fromp(), VarRef)->varScopep(), edgeType),
                    VAccess::READ));
                VL_DO_DANGLING(nodep->deleteTree(), nodep);
            }
        }
    }

    //--------------------
    virtual void visit(AstNode* nodep) override { iterateChildren(nodep); }

public:
    // CONSTRUCTORS
    explicit DynamicSchedulerCreateEventsVisitor(AstNetlist* nodep) { iterate(nodep); }
    virtual ~DynamicSchedulerCreateEventsVisitor() override {}
};

//######################################################################

class DynamicSchedulerAddTriggersVisitor final : public AstNVisitor {
private:
    // NODE STATE
    //  AstVar::user1()      -> bool.  Set true if variable is waited on
    // AstUser1InUse    m_inuser1;      (Allocated for use in DynamicSchedulerCreateEventsVisitor)
    //  AstNode::user2()      -> bool.  Set true if node has been processed
    AstUser2InUse m_inuser2;

    // STATE
    using VarMap = std::map<const std::pair<AstNodeModule*, string>, AstVar*>;
    VarMap m_modVarMap;  // Table of new var names created under module
    size_t m_count = 0;
    AstTopScope* m_topScopep = nullptr;  // Current top scope

    // METHODS
    VL_DEBUG_FUNC;  // Declare debug()

    AstVarScope* getCreateVar(AstVarScope* oldvarscp, const string& name) {
        UASSERT_OBJ(oldvarscp->scopep(), oldvarscp, "Var unscoped");
        AstVar* varp;
        AstNodeModule* addmodp = oldvarscp->scopep()->modp();
        // We need a new AstVar, but only one for all scopes, to match the new AstVarScope
        const auto it = m_modVarMap.find(make_pair(addmodp, name));
        if (it != m_modVarMap.end()) {
            // Created module's AstVar earlier under some other scope
            varp = it->second;
        } else {
            varp = new AstVar{oldvarscp->fileline(), AstVarType::BLOCKTEMP, name,
                              oldvarscp->varp()};
            varp->dtypeFrom(oldvarscp);
            addmodp->addStmtp(varp);
            m_modVarMap.emplace(make_pair(addmodp, name), varp);
        }
        AstVarScope* varscp = new AstVarScope{oldvarscp->fileline(), oldvarscp->scopep(), varp};
        oldvarscp->scopep()->addVarp(varscp);
        return varscp;
    }

    // VISITORS
    virtual void visit(AstTopScope* nodep) override {
        m_topScopep = nodep;
        iterateChildren(nodep);
    }
    virtual void visit(AstNodeAssign* nodep) override {
        if (nodep->user2SetOnce()) return;
        if (auto* varrefp = VN_CAST(nodep->lhsp(), VarRef)) {
            auto fl = nodep->fileline();
            if (varrefp->varp()->user1()) {
                auto* newvscp
                    = getCreateVar(varrefp->varScopep(), "__Vprevval" + std::to_string(m_count++)
                                                             + "__" + varrefp->name());
                AstNode* stmtspAfter = nullptr;

                if (auto* eventp = varrefp->varp()->edgeEvent(VEdgeType::ET_POSEDGE)) {
                    stmtspAfter = AstNode::addNext(
                        stmtspAfter,
                        new AstIf{
                            fl,
                            new AstAnd{
                                fl, new AstLogNot{fl, new AstVarRef{fl, newvscp, VAccess::READ}},
                                new AstVarRef{fl, varrefp->varScopep(), VAccess::READ}},
                            new AstEventTrigger{fl, new AstVarRef{fl, eventp, VAccess::WRITE}}});
                }

                if (auto* eventp = varrefp->varp()->edgeEvent(VEdgeType::ET_NEGEDGE)) {
                    stmtspAfter = AstNode::addNext(
                        stmtspAfter,
                        new AstIf{
                            fl,
                            new AstAnd{fl, new AstVarRef{fl, newvscp, VAccess::READ},
                                       new AstLogNot{fl, new AstVarRef{fl, varrefp->varScopep(),
                                                                       VAccess::READ}}},
                            new AstEventTrigger{fl, new AstVarRef{fl, eventp, VAccess::WRITE}}});
                }

                if (auto* eventp = varrefp->varp()->edgeEvent(VEdgeType::ET_ANYEDGE)) {
                    stmtspAfter = AstNode::addNext(
                        stmtspAfter,
                        new AstIf{
                            fl,
                            new AstNeq{fl, new AstVarRef{fl, newvscp, VAccess::READ},
                                       new AstVarRef{fl, varrefp->varScopep(), VAccess::READ}},
                            new AstEventTrigger{fl, new AstVarRef{fl, eventp, VAccess::WRITE}}});
                }

                if (stmtspAfter) {
                    auto* stmtspBefore
                        = new AstAssign{fl, new AstVarRef{fl, newvscp, VAccess::WRITE},
                                        new AstVarRef{fl, varrefp->varScopep(), VAccess::READ}};
                    if (!VN_IS(nodep, Assign)) {
                        nodep->addHereThisAsNext(new AstAlways{
                            nodep->fileline(), VAlwaysKwd::ALWAYS, nullptr, stmtspBefore});
                        nodep->addNextHere(new AstAlways{nodep->fileline(), VAlwaysKwd::ALWAYS,
                                                         nullptr, stmtspAfter});
                    } else {
                        nodep->addHereThisAsNext(stmtspBefore);
                        nodep->addNextHere(stmtspAfter);
                    }
                }
            }
        }
    }
    virtual void visit(AstVarScope* nodep) override {
        AstVar* varp = nodep->varp();
        if (varp->user1() && (varp->isUsedClock() || varp->isSigPublic())) {
            auto fl = nodep->fileline();
            for (auto edgeType :
                 {VEdgeType::ET_POSEDGE, VEdgeType::ET_NEGEDGE, VEdgeType::ET_ANYEDGE}) {
                // TODO: make sure ANYEDGE is checked before the others
                if (auto* eventp = varp->edgeEvent(edgeType)) {
                    auto* activep = new AstActive{
                        fl, "",
                        new AstSenTree{fl,
                                       new AstSenItem{fl,
                                                      edgeType == VEdgeType::ET_ANYEDGE
                                                          ? VEdgeType::ET_BOTHEDGE
                                                          : edgeType,
                                                      new AstVarRef{fl, nodep, VAccess::READ}}}};
                    m_topScopep->addSenTreep(activep->sensesp());
                    auto* ifp = new AstIf{
                        fl, new AstLogNot{fl, new AstVarRef{fl, eventp, VAccess::READ}},
                        new AstEventTrigger{fl, new AstVarRef{fl, eventp, VAccess::WRITE}}};
                    auto* alwaysp
                        = new AstAlways{nodep->fileline(), VAlwaysKwd::ALWAYS, nullptr, ifp};
                    activep->addStmtsp(alwaysp);
                    nodep->scopep()->addActivep(activep);
                }
            }
        }
    }

    //--------------------
    virtual void visit(AstNode* nodep) override { iterateChildren(nodep); }

public:
    // CONSTRUCTORS
    explicit DynamicSchedulerAddTriggersVisitor(AstNetlist* nodep) { iterate(nodep); }
    virtual ~DynamicSchedulerAddTriggersVisitor() override {}
};

//######################################################################
// DynamicScheduler class functions

void V3DynamicScheduler::handleAnyedge(AstNetlist* nodep) {
    DynamicSchedulerMarkDynamicVisitor visitor(nodep);
    { DynamicSchedulerMarkVariablesVisitor visitor(nodep); }
    UINFO(2, "  Handle Anyedge Sensitivity...\n");
    { DynamicSchedulerAnyedgeVisitor visitor(nodep); }
    V3Global::dumpCheckGlobalTree("dsch_anyedge", 0, v3Global.opt.dumpTreeLevel(__FILE__) >= 3);
}

void V3DynamicScheduler::transformProcesses(AstNetlist* nodep) {
    UINFO(2, "  Transform Intra Assign Delays...\n");
    { DynamicSchedulerIntraAssignDelayVisitor visitor(nodep); }
    DynamicSchedulerMarkDynamicVisitor visitor(nodep);
    { DynamicSchedulerMarkVariablesVisitor visitor(nodep); }
    UINFO(2, "  Wrap Processes...\n");
    { DynamicSchedulerWrapProcessVisitor visitor(nodep); }
    V3Global::dumpCheckGlobalTree("dsch_wrap", 0, v3Global.opt.dumpTreeLevel(__FILE__) >= 6);
    UINFO(2, "  Transform Dynamically Triggered Processes...\n");
    { DynamicSchedulerAlwaysVisitor visitor(nodep); }
    V3Global::dumpCheckGlobalTree("dsch_always", 0, v3Global.opt.dumpTreeLevel(__FILE__) >= 6);
    UINFO(2, "  Transform AssignDlys in Suspendable Processes...\n");
    { DynamicSchedulerAssignDlyVisitor visitor(nodep); }
    V3Global::dumpCheckGlobalTree("dsch_dly", 0, v3Global.opt.dumpTreeLevel(__FILE__) >= 6);
    UINFO(2, "  Move Forked Processes to New Functions...\n");
    { DynamicSchedulerForkVisitor visitor(nodep); }
    V3Global::dumpCheckGlobalTree("dsch_proc", 0, v3Global.opt.dumpTreeLevel(__FILE__) >= 3);
}

void V3DynamicScheduler::prepareEvents(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ": " << endl);
    UINFO(2, "  Add Edge Events...\n");
    DynamicSchedulerCreateEventsVisitor createEventsVisitor(nodep);
    V3Global::dumpCheckGlobalTree("dsch_make_events", 0,
                                  v3Global.opt.dumpTreeLevel(__FILE__) >= 6);
    UINFO(2, "  Add Edge Event Triggers...\n");
    DynamicSchedulerAddTriggersVisitor addTriggersVisitor(nodep);
    V3Global::dumpCheckGlobalTree("dsch_add_triggers", 0,
                                  v3Global.opt.dumpTreeLevel(__FILE__) >= 6);
    UINFO(2, "  Done.\n");
    V3Global::dumpCheckGlobalTree("dsch_prep_events", 0,
                                  v3Global.opt.dumpTreeLevel(__FILE__) >= 3);
}
