// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Bind classes declared in modules to their instance
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
// V3ClassContext's Transformations:
//
// Each class declared in a module or interface with several instances:
//      Add member __Vinstance (AstClass::instancep), a virtual interface
//      to the instance, set from a constructor argument (AstClass::instanceArgp)
//
// bindMembers, before V3Width:
//      Add __Vinstance and the constructor argument, defaulting to null
//      Hoist static members to the module, so each instance has its own.
//        Leave a non-static declaration in the class for V3Width
//      Convert references to members, generate block members, cells and
//        interface ports of the module to __Vinstance.<name>
//      Point inst.cls::method() to the method hoisted to inst
//
// bindCtorCalls, after V3Width as it needs the type of each new():
//      Pass the instance to each new()
//      Convert obj.member of a hoisted static to obj.__Vinstance.member
//      Delete the declarations left in classes
//
//*************************************************************************

#include "V3PchAstNoMT.h"  // VL_MT_DISABLED_CODE_UNIT

#include "V3ClassContext.h"

#include "V3MemberMap.h"

VL_DEFINE_DEBUG_FUNCTIONS;

//######################################################################

// Name of a static class member hoisted to the module
static string hoistedName(const AstClass* classp, const AstNode* memberp) {
    return classp->name() + "__Vstatic__" + memberp->name();
}

//######################################################################

class ClassContextMembersVisitor final : public VNVisitor {
    // NODE STATE
    //  AstVar::user1p()        -> AstVar*. Interface port's <port>__Vport variable
    //  AstVar::user2p()        -> AstNodeModule*. Module containing it, incl. generate blocks
    //  AstNodeFTask::user2p()  -> AstNodeModule*. Module containing it, incl. generate blocks
    const VNUser1InUse m_inuser1;
    const VNUser2InUse m_inuser2;

    // STATE
    VMemberMap m_memberMap;  // Members of modules
    AstNodeModule* m_modp = nullptr;  // Current module
    AstClass* m_classp = nullptr;  // Current class, if it has __Vinstance

    // METHODS
    // Return modules with several instances that contain classes
    std::vector<AstNodeModule*> modulesToBind(AstNetlist* nodep) {
        // V3Param put instantiating modules first
        std::unordered_map<const AstNodeModule*, int> counts;
        counts[nodep->topModulep()] = 1;
        for (AstNode* modp = nodep->modulesp(); modp; modp = modp->nextp()) {
            const int count = counts[VN_AS(modp, NodeModule)];
            if (!count) continue;
            modp->foreach([&](const AstCell* cellp) {
                // An array of instances counts as several
                counts[cellp->modp()] += cellp->rangep() ? 2 * count : count;
            });
        }
        std::vector<AstNodeModule*> modps;
        for (AstNode* itemp = nodep->modulesp(); itemp; itemp = itemp->nextp()) {
            AstNodeModule* const modp = VN_AS(itemp, NodeModule);
            if ((VN_IS(modp, Module) || VN_IS(modp, Iface)) && counts[modp] > 1
                && modp->exists([](const AstClass*) { return true; })) {
                modps.push_back(modp);
            }
        }
        return modps;
    }
    // Add __Viftop variable for the cell, as V3LinkCells does for interfaces
    void addCellVar(AstCell* cellp) {
        if (cellp->hasIfaceVar() || cellp->rangep()) return;
        const string name = cellp->name() + "__Viftop";
        AstIfaceRefDType* const dtypep
            = new AstIfaceRefDType{cellp->fileline(), cellp->name(), cellp->modp()->name()};
        dtypep->ifacep(cellp->modp());
        dtypep->cellp(cellp);
        AstVar* const varp
            = new AstVar{cellp->fileline(), VVarType::IFACEREF, name, VFlagChildDType{}, dtypep};
        cellp->addNextHere(varp);
        cellp->hasIfaceVar(true);
    }
    // Return __Vport variable holding the interface of a port; __Vinstance can't reach ports
    AstVar* portVarp(AstNodeModule* modp, AstVar* portp) {
        if (portp->user1p()) return VN_AS(portp->user1p(), Var);
        FileLine* const fl = portp->fileline();
        AstIfaceRefDType* const dtypep
            = VN_AS(portp->childDTypep(), IfaceRefDType)->cloneTree(false);
        dtypep->isVirtual(true);
        AstVar* const varp
            = new AstVar{fl, VVarType::VAR, portp->name() + "__Vport", VFlagChildDType{}, dtypep};
        varp->lifetime(VLifetime::STATIC_IMPLICIT);
        // First, so static initializers can use it
        modp->stmtsp()->addHereThisAsNext(
            new AstInitialStatic{fl, new AstAssign{fl, new AstVarRef{fl, varp, VAccess::WRITE},
                                                   new AstVarRef{fl, portp, VAccess::READ}}});
        modp->stmtsp()->addHereThisAsNext(varp);
        portp->user1p(varp);
        return varp;
    }
    // Create __Vinstance member or constructor argument
    AstVar* newInstVarp(AstNodeModule* modp, VVarType type) {
        FileLine* const fl = modp->fileline();
        AstIfaceRefDType* const dtypep = new AstIfaceRefDType{fl, "", modp->name()};
        dtypep->ifacep(modp);
        dtypep->isVirtual(true);
        AstVar* const varp = new AstVar{fl, type, "__Vinstance", VFlagChildDType{}, dtypep};
        varp->lifetime(VLifetime::AUTOMATIC_IMPLICIT);
        return varp;
    }
    // Hoist static class member to the module
    void hoistStatic(AstNodeModule* modp, AstClass* classp, AstNode* memberp) {
        UINFO(9, "Hoist static member " << memberp);
        // Leave a non-static declaration without body or value
        AstNode* const declp = memberp->cloneTree(false);
        if (AstVar* const varp = VN_CAST(declp, Var)) {
            varp->lifetime(VLifetime::AUTOMATIC_IMPLICIT);
            if (varp->valuep()) varp->valuep()->unlinkFrBack()->deleteTree();
        } else {
            AstNodeFTask* const ftaskp = VN_AS(declp, NodeFTask);
            ftaskp->isStatic(false);
            for (AstNode *stmtp = ftaskp->stmtsp(), *nextp; stmtp; stmtp = nextp) {
                nextp = stmtp->nextp();
                const AstVar* const portp = VN_CAST(stmtp, Var);
                if (!portp || !portp->isIO()) {
                    VL_DO_DANGLING(stmtp->unlinkFrBack()->deleteTree(), stmtp);
                }
            }
            ftaskp->fileline(new FileLine{ftaskp->fileline()});
            ftaskp->fileline()->warnOff(V3ErrorCode::NORETURN, true);
        }
        memberp->replaceWith(declp);
        memberp->name(hoistedName(classp, memberp));
        if (AstVar* const varp = VN_CAST(memberp, Var)) {
            varp->varType(VVarType::VAR);
        } else {
            AstNodeFTask* const ftaskp = VN_AS(memberp, NodeFTask);
            ftaskp->classMethod(false);
            ftaskp->isStatic(false);
            ftaskp->foreach([](AstVarRef* refp) {
                if (refp->varp()->isFuncLocal()) refp->classOrPackagep(nullptr);
            });
        }
        memberp->user2p(modp);
        modp->addStmtsp(memberp);
    }
    // Add __Vinstance to the class, hoist statics to the module
    void bindClass(AstNodeModule* modp, AstClass* classp) {
        UINFO(4, "Bind class " << classp << " to " << modp);
        AstVar* const instp = newInstVarp(modp, VVarType::MEMBER);
        AstVar* const argp = newInstVarp(modp, VVarType::PORT);
        argp->funcLocal(true);
        argp->direction(VDirection::INPUT);
        argp->valuep(new AstConst{modp->fileline(), AstConst::Null{}});
        for (AstNode *stmtp = classp->membersp(), *nextp; stmtp; stmtp = nextp) {
            nextp = stmtp->nextp();
            const AstVar* const varp = VN_CAST(stmtp, Var);
            AstNodeFTask* const ftaskp = VN_CAST(stmtp, NodeFTask);
            if (ftaskp && ftaskp->isConstructor()) {
                ftaskp->addDeclsp(argp);
            } else if (VN_IS(stmtp, InitialStatic)) {
                modp->addStmtsp(stmtp->unlinkFrBack());
            } else if ((varp && varp->lifetime().isStatic() && !varp->isParam())
                       || (ftaskp && ftaskp->isStatic())) {
                hoistStatic(modp, classp, stmtp);
            }
        }
        classp->prependMembersp(instp);
        classp->instancep(instp);
        classp->instanceArgp(argp);
    }
    AstNodeExpr* newInstRefp(FileLine* fl, VAccess access) {
        return new AstVarRef{fl, m_classp->instancep(), access};
    }
    // Convert dotted cell/port path to __Vinstance.<path>, or nullptr
    AstNodeExpr* newCellPathp(FileLine* fl, const string& dotted) {
        AstNodeExpr* pathp = newInstRefp(fl, VAccess::READ);
        AstNodeModule* modp = m_modp;
        string rest = dotted;
        while (!rest.empty()) {
            const size_t pos = rest.find('.');
            const string name = rest.substr(0, pos);
            rest = pos == string::npos ? "" : rest.substr(pos + 1);
            AstNode* const itemp = m_memberMap.findMember(modp, name);
            AstCell* const cellp = VN_CAST(itemp, Cell);
            AstVar* const portp = VN_CAST(itemp, Var);
            if (cellp) addCellVar(cellp);
            const AstIfaceRefDType* const portDTypep
                = portp && portp->isIfaceRef() ? VN_CAST(portp->childDTypep(), IfaceRefDType)
                                               : nullptr;
            if (portDTypep && portDTypep->ifacep()) {
                // Interface port
                pathp = new AstMemberRef{fl, pathp, VFlagChildDType{},
                                         portVarp(modp, portp)->name()};
                modp = portDTypep->ifacep();
            } else if (cellp && cellp->hasIfaceVar()) {
                pathp = new AstMemberRef{fl, pathp, VFlagChildDType{}, name};
                modp = cellp->modp();
            } else {
                VL_DO_DANGLING(pathp->deleteTree(), pathp);
                return nullptr;
            }
        }
        return pathp;
    }
    // True if member of the current module. Hoisted statics used elsewhere are unsupported
    bool isModuleMember(const AstNode* nodep, const AstNode* memberp) const {
        if (!memberp || !memberp->user2p()) return false;
        if (memberp->user2p() == m_modp) return true;
        nodep->v3warn(E_UNSUPPORTED, "Unsupported: static member of a class declared in a"
                                     " module with several instances, accessed outside"
                                     " the module");
        return false;
    }
    // Convert to __Vinstance.member
    void replaceByMemberRef(AstNodeVarRef* nodep) {
        UINFO(9, "Reference through __Vinstance " << nodep);
        FileLine* const fl = nodep->fileline();
        AstMemberRef* const newp = new AstMemberRef{fl, newInstRefp(fl, nodep->access()),
                                                    VFlagChildDType{}, nodep->varp()->name()};
        newp->varp(nodep->varp());
        newp->access(nodep->access());
        nodep->replaceWith(newp);
        VL_DO_DANGLING(pushDeletep(nodep), nodep);
    }

    // VISITORS
    void visit(AstNodeModule* nodep) override {
        VL_RESTORER(m_modp);
        m_modp = nodep;
        iterateChildren(nodep);
    }
    void visit(AstClass* nodep) override {
        VL_RESTORER(m_classp);
        m_classp = nodep->instancep() ? nodep : nullptr;
        iterateChildren(nodep);
    }
    void visit(AstVarRef* nodep) override {
        if (!isModuleMember(nodep, nodep->varp())) return;
        nodep->classOrPackagep(nullptr);
        if (m_classp) replaceByMemberRef(nodep);
    }
    void visit(AstVarXRef* nodep) override {
        if (!m_classp || nodep->dotted().empty()) return;
        // Generate block member, blk.member
        if (nodep->varp()->user2p() == m_modp) {
            replaceByMemberRef(nodep);
            return;
        }
        AstNodeExpr* const pathp = newCellPathp(nodep->fileline(), nodep->dotted());
        if (!pathp) return;
        AstMemberRef* const newp
            = new AstMemberRef{nodep->fileline(), pathp, VFlagChildDType{}, nodep->name()};
        newp->access(nodep->access());
        nodep->replaceWith(newp);
        VL_DO_DANGLING(pushDeletep(nodep), nodep);
    }
    // Not an AstNodeFTaskRef of the module, the name is relative to the object
    void visit(AstMethodCall* nodep) override { iterateChildren(nodep); }
    void visit(AstNodeFTaskRef* nodep) override {
        iterateChildren(nodep);
        AstNodeFTask* const taskp = nodep->taskp();
        if (!nodep->dotted().empty()) {
            // inst.cls::f(), call the static hoisted to inst
            if (taskp && taskp->user2p()) {
                nodep->name(taskp->name());
                nodep->classOrPackagep(nullptr);
                return;
            }
            // cell.f() or port.f() in a class
            if (!m_classp || !taskp) return;
            AstNodeExpr* const pathp = newCellPathp(nodep->fileline(), nodep->dotted());
            if (!pathp) return;
            AstArg* const argsp
                = nodep->argsp() ? nodep->argsp()->unlinkFrBackWithNext() : nullptr;
            AstMethodCall* const newp = new AstMethodCall{nodep->fileline(), pathp,
                                                          VFlagChildDType{}, taskp->name(), argsp};
            newp->taskp(taskp);
            nodep->replaceWith(newp);
            VL_DO_DANGLING(pushDeletep(nodep), nodep);
            return;
        }
        if (!isModuleMember(nodep, taskp)) return;
        nodep->name(taskp->name());
        nodep->classOrPackagep(nullptr);
        if (!m_classp) return;
        AstArg* const argsp = nodep->argsp() ? nodep->argsp()->unlinkFrBackWithNext() : nullptr;
        AstMethodCall* const newp
            = new AstMethodCall{nodep->fileline(), newInstRefp(nodep->fileline(), VAccess::READ),
                                VFlagChildDType{}, taskp->name(), argsp};
        newp->taskp(taskp);
        nodep->replaceWith(newp);
        VL_DO_DANGLING(pushDeletep(nodep), nodep);
    }
    void visit(AstNew* nodep) override {
        iterateChildren(nodep);
        // super.new, pass __Vinstance to the base class
        const AstClass* const basep = VN_CAST(nodep->classOrPackagep(), Class);
        if (!m_classp || !nodep->taskp() || !basep || !basep->instancep()) return;
        FileLine* const fl = nodep->fileline();
        nodep->addArgsp(
            new AstArg{fl, basep->instanceArgp()->name(), newInstRefp(fl, VAccess::READ)});
    }
    void visit(AstNode* nodep) override { iterateChildren(nodep); }

public:
    // CONSTRUCTORS
    explicit ClassContextMembersVisitor(AstNetlist* nodep) {
        const std::vector<AstNodeModule*> modps = modulesToBind(nodep);
        if (modps.empty()) return;
        // Cell variables, for inst.obj = new
        const std::unordered_set<const AstNodeModule*> instModps{modps.begin(), modps.end()};
        nodep->foreach([&](AstCell* cellp) {
            if (instModps.count(cellp->modp())) addCellVar(cellp);
        });
        for (AstNodeModule* const modp : modps) {
            const auto markMembers = [modp](AstNode* stmtsp) {
                for (AstNode* stmtp = stmtsp; stmtp; stmtp = stmtp->nextp()) {
                    const AstVar* const varp = VN_CAST(stmtp, Var);
                    if ((varp && !varp->isParam()) || VN_IS(stmtp, NodeFTask)) {
                        stmtp->user2p(modp);
                    }
                }
            };
            markMembers(modp->stmtsp());
            modp->foreach([&](AstGenBlock* blockp) { markMembers(blockp->itemsp()); });
            std::vector<AstClass*> classps;
            modp->foreach([&](AstClass* classp) {
                if (!classp->isInterfaceClass() && !classp->isCovergroup()) {
                    classps.push_back(classp);
                }
            });
            for (AstClass* const classp : classps) bindClass(modp, classp);
        }
        iterate(nodep);
    }
    ~ClassContextMembersVisitor() override = default;
};

//######################################################################

class ClassContextCtorVisitor final : public VNVisitor {
    // NODE STATE
    //  AstNodeModule::user1p() -> AstVar*. __Vself, the instance itself
    //  AstClass::user2p()      -> AstNodeModule*. Module containing the class
    //  AstVar::user3p()        -> AstVar*. Declaration left in class: hoisted member
    //  AstNodeFTask::user3p()  -> AstNodeFTask*. Declaration left in class: hoisted member
    //  AstVar::user4p()        -> AstClass*. Declaration left in class: its class
    //  AstNodeFTask::user4p()  -> AstClass*. Declaration left in class: its class
    const VNUser1InUse m_inuser1;
    const VNUser2InUse m_inuser2;
    const VNUser3InUse m_inuser3;
    const VNUser4InUse m_inuser4;

    // STATE
    VMemberMap m_memberMap;  // Members of modules
    AstNodeModule* m_modp = nullptr;  // Current module
    AstClass* m_classp = nullptr;  // Current class
    std::vector<AstNode*> m_declps;  // Declarations left in classes

    // METHODS
    AstVar* selfVarp(AstNodeModule* modp, AstNodeDType* dtypep) {
        if (!modp->user1p()) {
            AstVar* const varp
                = new AstVar{modp->fileline(), VVarType::IFACEREF, "__Vself", dtypep};
            varp->lifetime(VLifetime::STATIC_IMPLICIT);
            modp->addStmtsp(varp);
            modp->user1p(varp);
        }
        return VN_AS(modp->user1p(), Var);
    }
    // Return instance of inst.obj = new or vif.obj = new, or nullptr
    AstNodeExpr* newAssignedInstp(const AstNew* nodep, const AstNodeModule* instModp) {
        const AstNodeAssign* const assignp = VN_CAST(nodep->backp(), NodeAssign);
        if (!assignp || assignp->rhsp() != nodep) return nullptr;
        if (const AstVarXRef* const refp = VN_CAST(assignp->lhsp(), VarXRef)) {
            // a.l.obj = new, find variable of cell or port l in a
            const string& dotted = refp->dotted();
            const size_t pos = dotted.rfind('.');
            const string prefix = pos == string::npos ? "" : dotted.substr(0, pos);
            const string name = pos == string::npos ? dotted : dotted.substr(pos + 1);
            const AstNodeModule* modp = m_modp;
            for (string rest = prefix; !rest.empty();) {
                const size_t dot = rest.find('.');
                const AstCell* const cellp
                    = VN_CAST(m_memberMap.findMember(modp, rest.substr(0, dot)), Cell);
                if (!cellp) return nullptr;
                rest = dot == string::npos ? "" : rest.substr(dot + 1);
                modp = cellp->modp();
            }
            for (const string& varName : {name, name + "__Viftop"}) {
                AstVar* const varp = VN_CAST(m_memberMap.findMember(modp, varName), Var);
                const AstIfaceRefDType* const dtypep
                    = varp ? VN_CAST(varp->dtypep()->skipRefp(), IfaceRefDType) : nullptr;
                if (!dtypep || dtypep->ifaceViaCellp() != instModp) continue;
                if (prefix.empty()) return new AstVarRef{nodep->fileline(), varp, VAccess::READ};
                return new AstVarXRef{nodep->fileline(), varp, prefix, VAccess::READ};
            }
        } else if (const AstMemberRef* const selp = VN_CAST(assignp->lhsp(), MemberRef)) {
            // vif.obj = new or vifs[i].obj = new
            const AstIfaceRefDType* const dtypep
                = VN_CAST(selp->fromp()->dtypep()->skipRefp(), IfaceRefDType);
            if (dtypep && dtypep->ifaceViaCellp() == instModp) {
                AstNodeExpr* const instp = selp->fromp()->cloneTree(false);
                instp->foreach([](AstNodeVarRef* refp) { refp->access(VAccess::READ); });
                return instp;
            }
        }
        return nullptr;
    }
    // Module containing the class of a declaration left in a class
    AstNodeModule* declModp(const AstNode* declp) const {
        return VN_AS(VN_AS(declp->user4p(), Class)->user2p(), NodeModule);
    }
    // Create fromp.__Vinstance for a declaration left in a class
    AstMemberRef* newInstSelp(AstNodeExpr* fromp, const AstNode* declp, VAccess access) {
        AstMemberRef* const selp = new AstMemberRef{fromp->fileline(), fromp,
                                                    VN_AS(declp->user4p(), Class)->instancep()};
        selp->access(access);
        return selp;
    }

    // VISITORS
    void visit(AstNodeModule* nodep) override {
        VL_RESTORER(m_modp);
        m_modp = nodep;
        iterateChildren(nodep);
    }
    void visit(AstClass* nodep) override {
        VL_RESTORER(m_classp);
        m_classp = nodep;
        iterateChildren(nodep);
    }
    void visit(AstNew* nodep) override {
        iterateChildren(nodep);
        const AstClass* const classp = VN_CAST(nodep->classOrPackagep(), Class);
        const AstVar* const instp = classp ? classp->instancep() : nullptr;
        // super.new done by bindMembers
        if (!instp || !VN_IS(nodep->dtypep()->skipRefp(), ClassRefDType)) return;
        AstNodeModule* const modp = VN_AS(classp->user2p(), NodeModule);
        AstVar* const classInstp = m_classp ? m_classp->instancep() : nullptr;
        FileLine* const fl = nodep->fileline();
        AstNodeExpr* instExprp = nullptr;
        if (classInstp && m_classp->user2p() == modp) {
            instExprp = new AstVarRef{fl, classInstp, VAccess::READ};
        } else if (!m_classp && m_modp == modp) {
            instExprp = new AstVarRef{fl, selfVarp(modp, instp->dtypep()), VAccess::READ};
        } else {
            instExprp = newAssignedInstp(nodep, modp);
        }
        if (!instExprp) {
            nodep->v3warn(E_UNSUPPORTED, "Unsupported: 'new' of a class declared in a module"
                                         " with several instances, outside the module");
            return;
        }
        UINFO(9, "Pass instance to " << nodep);
        // The last argument is the instance
        AstArg* argp = nodep->argsp();
        UASSERT_OBJ(argp, nodep, "'new' without the instance argument");
        while (argp->nextp()) argp = VN_AS(argp->nextp(), Arg);
        pushDeletep(argp->exprp()->unlinkFrBack());
        argp->exprp(instExprp);
    }
    // obj.member of a hoisted static variable
    void visit(AstMemberRef* nodep) override {
        iterateChildren(nodep);
        AstVar* const declp = nodep->varp();
        if (!declp || !declp->user3p()) return;
        AstVar* const varp = VN_AS(declp->user3p(), Var);
        AstMemberRef* const newp = new AstMemberRef{
            nodep->fileline(), newInstSelp(nodep->fromp()->unlinkFrBack(), declp, nodep->access()),
            varp};
        newp->access(nodep->access());
        varp->sensIfacep(declModp(declp));
        nodep->replaceWith(newp);
        VL_DO_DANGLING(pushDeletep(nodep), nodep);
    }
    // obj.method() of a hoisted static method
    void visit(AstMethodCall* nodep) override {
        iterateChildren(nodep);
        AstNodeFTask* const declp = nodep->taskp();
        if (!declp || !declp->user3p()) return;
        AstNodeFTask* const ftaskp = VN_AS(declp->user3p(), NodeFTask);
        AstArg* const argsp = nodep->argsp() ? nodep->argsp()->unlinkFrBackWithNext() : nullptr;
        AstMethodCall* const newp = new AstMethodCall{
            nodep->fileline(), newInstSelp(nodep->fromp()->unlinkFrBack(), declp, VAccess::READ),
            VFlagChildDType{}, ftaskp->name(), argsp};
        newp->taskp(ftaskp);
        newp->dtypeFrom(nodep);
        newp->classOrPackagep(declModp(declp));
        nodep->replaceWith(newp);
        VL_DO_DANGLING(pushDeletep(nodep), nodep);
    }
    void visit(AstNode* nodep) override { iterateChildren(nodep); }

public:
    // CONSTRUCTORS
    explicit ClassContextCtorVisitor(AstNetlist* nodep) {
        for (AstNode* modp = nodep->modulesp(); modp; modp = modp->nextp()) {
            modp->foreach([&](AstClass* classp) {
                if (!classp->instancep()) return;
                classp->user2p(modp);
                for (AstNode* stmtp = classp->membersp(); stmtp; stmtp = stmtp->nextp()) {
                    if (!VN_IS(stmtp, Var) && !VN_IS(stmtp, NodeFTask)) continue;
                    AstNode* const memberp
                        = m_memberMap.findMember(modp, hoistedName(classp, stmtp));
                    if (!memberp) continue;
                    stmtp->user3p(memberp);
                    stmtp->user4p(classp);
                    m_declps.push_back(stmtp);
                }
            });
        }
        iterate(nodep);
        // All references now point to the hoisted members
        for (AstNode* const declp : m_declps) {
            VL_DO_DANGLING(pushDeletep(declp->unlinkFrBack()), declp);
        }
    }
    ~ClassContextCtorVisitor() override = default;
};

//######################################################################
// V3ClassContext class functions

void V3ClassContext::bindMembers(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { ClassContextMembersVisitor{nodep}; }  // Destruct before checking
    V3Global::dumpCheckGlobalTree("classcontext", 0, dumpTreeEitherLevel() >= 3);
}

void V3ClassContext::bindCtorCalls(AstNetlist* nodep) {
    UINFO(2, __FUNCTION__ << ":");
    { ClassContextCtorVisitor{nodep}; }  // Destruct before checking
    V3Global::dumpCheckGlobalTree("classcontextctor", 0, dumpTreeEitherLevel() >= 3);
}
