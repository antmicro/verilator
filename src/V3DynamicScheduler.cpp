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

static AstVarScope* getCreateEvent(AstVarScope* vscp, VEdgeType edgeType) {
    UASSERT_OBJ(vscp->scopep(), vscp, "Var unscoped");
    if (auto* eventp = vscp->varp()->edgeEvent(edgeType)) return eventp;
    string newvarname = (string("__VedgeEvent__") + vscp->scopep()->nameDotless() + "__"
                         + edgeType.ascii() + "__" + vscp->varp()->name());
    auto* newvarp = new AstVar(vscp->fileline(), AstVarType::VAR, newvarname,
                               vscp->findBasicDType(AstBasicDTypeKwd::EVENTVALUE));
    newvarp->sigPublic(true);
    vscp->scopep()->modp()->addStmtp(newvarp);
    auto* newvscp = new AstVarScope(vscp->fileline(), vscp->scopep(), newvarp);
    vscp->user1p(newvscp);
    vscp->scopep()->addVarp(newvscp);
    vscp->varp()->edgeEvent(edgeType, newvscp);
    return newvscp;
}

//######################################################################

class DynamicSchedulerMarkDynamicVisitor final : public AstNVisitor {
private:
    // NODE STATE
    // AstNode::user1()      -> bool.  Set true if process/function uses constructs like delays,
    // timing controls, waits, forks
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
                    nodep->user1u(true);
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
                    nodep->user1u(true);
                else
                    iterateChildren(nodep);
                if (nodep->user1()) repeat = true;
            }
        }
    }
    virtual void visit(AstDelay* nodep) override {
        if (m_proc) m_proc->user1u(true);
    }
    virtual void visit(AstTimingControl* nodep) override {
        if (m_proc) m_proc->user1u(true);
    }
    virtual void visit(AstWait* nodep) override {
        if (m_proc) m_proc->user1u(true);
    }
    virtual void visit(AstFork* nodep) override {
        if (m_proc) m_proc->user1u(m_proc->user1() || !nodep->joinType().joinNone());
    }
    virtual void visit(AstNodeFTaskRef* nodep) override {
        if (m_proc && nodep->taskp()->user1()) m_proc->user1u(true);
    }
    virtual void visit(AstMethodCall* nodep) override {
        if (m_proc && nodep->taskp()->user1()) m_proc->user1u(true);
    }
    virtual void visit(AstCMethodCall* nodep) override {
        if (m_proc && nodep->funcp()->user1()) m_proc->user1u(true);
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
    // AstVar::user1()      -> bool.  Set true if variable is written to from a 'dynamic'
    // process/function AstUser1InUse    m_inuser1;      (Allocated for use in
    // DynamicSchedulerMarkDynamicVisitor)

    // STATE
    bool m_dynamic = false;

    // METHODS
    VL_DEBUG_FUNC;  // Declare debug()

    // VISITORS
    virtual void visit(AstNodeProcedure* nodep) override {
        m_dynamic = nodep->user1();
        if (m_dynamic) iterateChildren(nodep);
        m_dynamic = false;
    }
    virtual void visit(AstNodeFTask* nodep) override {
        m_dynamic = nodep->user1();
        if (m_dynamic) iterateChildren(nodep);
        m_dynamic = false;
    }
    virtual void visit(AstCFunc* nodep) override {
        m_dynamic = nodep->user1();
        if (m_dynamic) iterateChildren(nodep);
        m_dynamic = false;
    }
    virtual void visit(AstVarRef* nodep) override {
        if (nodep->access().isWriteOrRW()) { nodep->varp()->user1(m_dynamic); }
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
    // AstVar::user1()      -> bool.  True if variable is written to from a 'dynamic'
    // process/function AstUser1InUse    m_inuser1;      (Allocated for use in
    // DynamicSchedulerMarkDynamicVisitor)

    // STATE

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
        else if (nodep->varrefp() && !nodep->varrefp()->varp()->user1()
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
    // AstNodeProcedure::user1()      -> bool.  Set true if 'dynamic' (shouldn't be split up)
    // AstUser1InUse    m_inuser1;      (Allocated for use in DynamicSchedulerMarkDynamicVisitor)

    // STATE

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

class DynamicSchedulerAlwaysVisitor final : public AstNVisitor {
private:
    // NODE STATE
    // AstNodeProcedure::user1()      -> bool.  Set true if shouldn't be split up
    // AstUser1InUse    m_inuser1;      (Allocated for use in DynamicSchedulerMarkDynamicVisitor)

    // STATE
    AstScope* m_scopep = nullptr;
    size_t m_count = 0;

    // METHODS
    VL_DEBUG_FUNC;  // Declare debug()

    AstCFunc* makeTopFunction(const string& name, bool slow = false) {
        AstCFunc* const funcp = new AstCFunc{m_scopep->fileline(), name, m_scopep};
        funcp->dontCombine(true);
        funcp->isStatic(false);
        funcp->isLoose(true);
        funcp->entryPoint(true);
        funcp->slow(slow);
        funcp->isConst(false);
        funcp->declPrivate(true);
        m_scopep->addActivep(funcp);
        return funcp;
    }

    // VISITORS
    virtual void visit(AstScope* nodep) override {
        m_scopep = nodep;
        iterateChildren(nodep);
        m_scopep = nullptr;
    }
    virtual void visit(AstAlways* nodep) override {
        auto* sensesp = nodep->sensesp();
        if (sensesp && sensesp->sensesp()
            && sensesp->sensesp()->edgeType() == VEdgeType::ET_ANYEDGE
            && sensesp->sensesp()->varrefp() && sensesp->sensesp()->varrefp()->varp()->user1()
            && nodep->bodysp()) {
            auto* eventp = getCreateEvent(sensesp->sensesp()->varrefp()->varScopep(),
                                          VEdgeType::ET_ANYEDGE);

            auto newFuncName = "_anyedge_" + std::to_string(m_count++);
            auto* newFuncpr
                = new AstCFunc(nodep->fileline(), newFuncName, m_scopep, "CoroutineTask");
            newFuncpr->isStatic(false);
            newFuncpr->isLoose(true);
            m_scopep->addActivep(newFuncpr);

            auto* beginp
                = new AstBegin(nodep->fileline(), "", nodep->bodysp()->unlinkFrBackWithNext());
            auto* forkp = new AstFork(nodep->fileline(), "", beginp);
            forkp->joinType(VJoinType::JOIN_NONE);
            auto* whilep = new AstWhile(
                nodep->fileline(), new AstConst(nodep->fileline(), AstConst::BitTrue()),
                new AstTimingControl(nodep->fileline(), sensesp->cloneTree(false), forkp));
            newFuncpr->addStmtsp(whilep);
            whilep->fileline()->warnOff(V3ErrorCode::INFINITELOOP, true);
            AstCCall* const callp = new AstCCall(nodep->fileline(), newFuncpr);
            auto* initialp = new AstInitial{nodep->fileline(), callp};
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

class DynamicSchedulerAssignDlyVisitor final : public AstNVisitor {
private:
    // NODE STATE
    // AstNodeProcedure::user1()      -> bool.  Set true if shouldn't be split up
    // AstUser1InUse    m_inuser1;      (Allocated for use in DynamicSchedulerMarkDynamicVisitor)
    // AstNode::user2()      -> bool.  Set true if node has been processed
    AstUser2InUse m_inuser2;

    // STATE
    AstVarScope* m_dlyEvent = nullptr;
    AstScope* m_scopep = nullptr;
    bool m_dynamic = false;

    // METHODS
    VL_DEBUG_FUNC;  // Declare debug()

    AstVarScope* getCreateDlyEvent() {
        if (m_dlyEvent) return m_dlyEvent;
        string newvarname = "__VdlyEvent__";
        auto* newvarp = new AstVar(m_scopep->fileline(), AstVarType::MODULETEMP, newvarname,
                                   m_scopep->findBasicDType(AstBasicDTypeKwd::EVENTVALUE));
        m_scopep->modp()->addStmtp(newvarp);
        auto* newvscp = new AstVarScope(m_scopep->fileline(), m_scopep, newvarp);
        m_scopep->addVarp(newvscp);
        return m_dlyEvent = newvscp;
    }

    // VISITORS
    virtual void visit(AstScope* nodep) override {
        m_scopep = nodep;
        iterateChildren(nodep);
        m_scopep = nullptr;
    }
    virtual void visit(AstNodeProcedure* nodep) override {
        m_dynamic = nodep->user1();
        if (m_dynamic) iterateChildren(nodep);
        m_dynamic = false;
    }
    virtual void visit(AstNodeFTask* nodep) override {
        m_dynamic = nodep->user1();
        if (m_dynamic) iterateChildren(nodep);
        m_dynamic = false;
    }
    virtual void visit(AstCFunc* nodep) override {
        m_dynamic = nodep->user1();
        if (m_dynamic) iterateChildren(nodep);
        m_dynamic = false;
    }
    virtual void visit(AstAssignDly* nodep) override {
        if (!m_dynamic) return;
        auto fl = nodep->fileline();
        auto* eventp = getCreateDlyEvent();
        auto* assignp
            = new AstAssign(fl, nodep->lhsp()->unlinkFrBack(), nodep->rhsp()->unlinkFrBack());
        auto* timingControlp = new AstTimingControl{
            fl,
            new AstSenTree{fl, new AstSenItem{fl, VEdgeType::ET_ANYEDGE,
                                              new AstVarRef{fl, eventp, VAccess::READ}}},
            assignp};
        auto* forkp = new AstFork(nodep->fileline(), "", timingControlp);
        forkp->joinType(VJoinType::JOIN_NONE);
        nodep->replaceWith(forkp);
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

public:
private:
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
            nodep->varp()->user1u(1);
            m_waitVars.insert(nodep->varScopep());
        } else if (m_inTimingControlSens) {
            if (!nodep->varp()->dtypep()->basicp()->isEventValue()) {
                nodep->varp()->user1u(1);
                auto edgeType = VN_CAST(nodep->backp(), SenItem)->edgeType();
                nodep->varScopep(getCreateEvent(nodep->varScopep(), edgeType));
                nodep->varp(nodep->varScopep()->varp());
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
    // AstVar::user1()      -> bool.  Set true if variable is waited on
    // AstUser1InUse    m_inuser1;      (Allocated for use in DynamicSchedulerCreateEventsVisitor)
    // AstNode::user2()      -> bool.  Set true if node has been processed
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
    { DynamicSchedulerAnyedgeVisitor visitor(nodep); }
    V3Global::dumpCheckGlobalTree("dsch_anyedge", 0, v3Global.opt.dumpTreeLevel(__FILE__) >= 3);
}

void V3DynamicScheduler::transformProcesses(AstNetlist* nodep) {
    DynamicSchedulerMarkDynamicVisitor visitor(nodep);
    { DynamicSchedulerWrapProcessVisitor visitor(nodep); }
    V3Global::dumpCheckGlobalTree("dsch_wrap", 0, v3Global.opt.dumpTreeLevel(__FILE__) >= 6);
    { DynamicSchedulerMarkVariablesVisitor visitor(nodep); }
    { DynamicSchedulerAlwaysVisitor visitor(nodep); }
    V3Global::dumpCheckGlobalTree("dsch_always", 0, v3Global.opt.dumpTreeLevel(__FILE__) >= 6);
    { DynamicSchedulerAssignDlyVisitor visitor(nodep); }
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
