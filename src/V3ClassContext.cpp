// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Bind class runtime owners
//
// Code available from: https://verilator.org
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of either the GNU Lesser General Public License Version 3
// or the Perl Artistic License Version 2.0.
// SPDX-FileCopyrightText: 2026 Wilson Snyder
// SPDX-License-Identifier: LGPL-3.0-only OR Artistic-2.0
//
//*************************************************************************
// Before expression lifting, turn external class dependencies into explicit receivers.
// Scope-address placeholders travel through ordinary scoped linking, including
// interface aliases. After linking, replace them with typed scope addresses and
// resolve concrete targets. Preparation prohibits target inlining after requirements
// reach their fixed point, even if optimization removes construction addresses.
// Method bodies contain only object-relative captures; concrete addresses occur
// at construction and static initialization.
// Captures are non-owning: module storage lives for the entire model lifetime.
// Constructor dependencies propagate until stable, including nested and base
// construction. Base-only inputs need no field in the derived subobject.
// Static routines keep their lexical declarations and receive their own hidden
// inputs; no object receiver exists in their bodies. Instance-owned static fields
// move before scoping, through V3Class's single package factory.
// Static selections use the receiver's type binding, never the handle value.
// Class-level generated processes live with static state; they have no object receiver.
//*************************************************************************

#include "V3PchAstNoMT.h"

#include "V3ClassContext.h"

#include "V3Class.h"
#include "V3Stats.h"

VL_DEFINE_DEBUG_FUNCTIONS;

namespace {
using CaptureKey = std::pair<string, AstNodeModule*>;
struct Capture final {
    AstNodeModule* anchorp = nullptr;  // Lexical origin of an owner-relative path
    bool retained = false;
    AstVar* fieldp = nullptr;
    AstVar* argp = nullptr;
};
struct ClassContext final {
    AstClass* classp = nullptr;
    bool object = true;  // Object constructor context, otherwise a static routine
    AstNodeModule* ownerp = nullptr;
    AstNodeFTask* ctorp = nullptr;
    AstClassPackage* packagep = nullptr;
    string storagePath;
    std::map<CaptureKey, Capture> captures;
    std::vector<std::pair<AstNodeVarRef*, CaptureKey>> vars;
    std::vector<std::pair<AstNodeVarRef*, CaptureKey>> ifaces;
    std::vector<std::pair<AstNodeFTaskRef*, CaptureKey>> calls;
    std::vector<AstNew*> constructions;
    std::vector<AstNodeProcedure*> processes;
};
struct StaticCall final {
    AstNodeFTaskRef* refp;
    ClassContext* callerp;
    AstNodeModule* ownerp;
    string path;
};
struct StaticSelection final {
    AstMemberSel* refp;
    ClassContext* callerp;
    AstNodeModule* ownerp;
    CaptureKey key;
};

class ContextPrepareVisitor final : public VNVisitor {
    AstNetlist* const m_rootp;  // Netlist being processed
    AstNodeModule* m_modp = nullptr;  // Current module, package, or class
    AstNodeModule* m_ownerp = nullptr;  // Current non-class lexical owner
    AstClass* m_classp = nullptr;  // Current class
    ClassContext* m_contextp = nullptr;  // Current class or static routine capture context
    std::map<AstNode*, AstNodeModule*> m_declarations;
    std::map<AstClass*, ClassContext> m_classes;
    std::vector<AstClass*> m_classps;  // Classes in traversal order
    std::map<AstNodeFTask*, ClassContext> m_routines;
    std::vector<ClassContext*> m_contexts;  // Class and static routine contexts in traversal order
    std::map<AstNodeModule*, AstVar*> m_markers;
    std::map<AstNodeModule*, string> m_instancePaths;  // Empty for ambiguous owners or the wrapper
    std::set<string> m_rootScopes;  // Already absolute dependency paths
    std::vector<std::pair<AstNew*, AstNodeModule*>> m_constructions;
    std::vector<StaticSelection> m_staticSelections;  // Static member selections needing receivers
    std::vector<std::pair<AstNodeVarRef*, AstNodeModule*>> m_staticVariables;
    std::vector<StaticCall> m_staticCalls;  // Static routine calls needing capture arguments
    std::vector<std::pair<AstNodeFTaskRef*, AstNodeModule*>> m_externalCalls;
    bool m_collect = true;  // Collect declarations and contexts before analyzing dependencies

    void indexInstancePaths(AstNodeModule* modp, const string& path) {
        const auto it = m_instancePaths.emplace(modp, path);
        // A module definition alone cannot identify one of several instances.
        if (!it.second) it.first->second.clear();
        for (AstNode* stmtp = modp->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            if (const AstCell* const cellp = VN_CAST(stmtp, Cell)) {
                indexInstancePaths(cellp->modp(), VString::dot(path, ".", cellp->name()));
            }
        }
    }

    static AstStmtExpr* newReceiverStmt(AstNodeExpr* exprp) {
        FileLine* const fl = new FileLine{exprp->fileline()};
        // The compiler intentionally discards the receiver value, like a void cast.
        fl->warnOff(V3ErrorCode::IGNOREDRETURN, true);
        return new AstStmtExpr{fl, exprp};
    }

    static void transferCall(const AstNodeFTaskRef* fromp, AstNodeFTaskRef* top,
                             AstNodeModule* ownerp) {
        top->taskp(fromp->taskp());
        top->classOrPackagep(ownerp);
        top->dtypeFrom(fromp);
        top->pli(fromp->pli());
        top->containsGenBlock(fromp->containsGenBlock());
        top->superReference(fromp->superReference());
        if (fromp->scopeNamep()) top->scopeNamep(fromp->scopeNamep()->unlinkFrBack());
        if (fromp->withp()) top->withp(fromp->withp()->unlinkFrBack());
    }

    void prepareStorage() {
        unsigned index = 0;
        for (AstClass* const classp : m_classps) {
            ClassContext& context = m_classes.at(classp);
            // Package classes already have singleton static storage and lexical scoping.
            if (!V3ClassContext::instanceOwnerp(classp)) continue;
            std::vector<AstNode*> moves;
            for (AstNode* nodep = classp->membersp(); nodep; nodep = nodep->nextp()) {
                if (const AstVar* const varp = VN_CAST(nodep, Var)) {
                    if (varp->lifetime().isStatic() && !varp->isParam()) moves.push_back(nodep);
                }
            }
            // Assertion monitors can also be nested in methods before scoping hoists them.
            moves.insert(moves.end(), context.processes.begin(), context.processes.end());
            if (moves.empty()) continue;
            AstClassPackage* const packagep
                = V3Class::newPackage(classp, "__VclassStatic" + cvtToStr(index) + "__Vclpkg");
            packagep->classCaptureTarget(true);
            context.packagep = packagep;
            context.storagePath = "__VclassStatic" + cvtToStr(index++);
            m_rootp->addModulesp(packagep);
            AstCell* const cellp = new AstCell{classp->fileline(),
                                               classp->fileline(),
                                               context.storagePath,
                                               packagep->name(),
                                               nullptr,
                                               nullptr,
                                               nullptr};
            cellp->modp(packagep);
            (VN_IS(context.ownerp, Package) ? m_rootp->topModulep() : context.ownerp)
                ->addStmtsp(cellp);
            for (AstNode* const movep : moves) {
                packagep->addStmtsp(movep->unlinkFrBack());
                m_declarations[movep] = packagep;
                movep->foreach([classp](AstVarRef* refp) {
                    if (refp->classOrPackagep() == classp && !refp->varp()->isFuncLocal()) {
                        refp->classOrPackagep(nullptr);
                    }
                });
            }
        }
    }

    AstVar* marker(AstNodeModule* modp) {
        AstVar*& varp = m_markers[modp];
        if (!varp) {
            AstModuleRefDType* const dtypep = new AstModuleRefDType{modp->fileline(), modp};
            m_rootp->typeTablep()->addTypesp(dtypep);
            varp = new AstVar{modp->fileline(), VVarType::MODULETEMP,
                              "__VclassContext" + cvtToStr(m_markers.size()) + "p", dtypep};
            varp->classContextMarker(true);
            modp->addStmtsp(varp);
        }
        return varp;
    }
    bool require(ClassContext& context, const CaptureKey& key, bool retained,
                 AstNodeModule* anchorp = nullptr) {
        const auto it = context.captures.emplace(key, Capture{});
        const bool changed = it.second || (retained && !it.first->second.retained);
        it.first->second.retained |= retained;
        if (it.second) it.first->second.anchorp = anchorp ? anchorp : context.ownerp;
        return changed;
    }
    CaptureKey dependency(AstNodeModule* modp, const string& path) { return {path, modp}; }
    AstClassRefDType* constructedType(AstNew* nodep, AstClass* classp) {
        if (AstClassRefDType* const dtypep = VN_CAST(nodep->dtypep()->skipRefp(), ClassRefDType)) {
            return dtypep;
        }
        UASSERT_OBJ(classp && classp->extendsp(), nodep, "Super construction without base");
        return VN_AS(classp->extendsp()->dtypep()->skipRefp(), ClassRefDType);
    }
    CaptureKey constructionKey(const CaptureKey& key, const AstClassRefDType* dtypep) {
        const string path = dtypep->ownerp() ? dtypep->ownerp()->path() : "";
        return {VString::dot(path, ".", key.first), key.second};
    }
    string receiverScopePath(const AstNodeExpr* nodep) {
        if (const AstVarXRef* const refp = VN_CAST(nodep, VarXRef)) {
            return refp->dotted();
        } else if (const AstMethodCall* const callp = VN_CAST(nodep, MethodCall)) {
            return receiverPath(callp->fromp(), VN_AS(m_declarations.at(callp->taskp()), Class));
        } else if (const AstMemberSel* const selp = VN_CAST(nodep, MemberSel)) {
            return receiverPath(selp->fromp(), VN_AS(m_declarations.at(selp->varp()), Class));
        } else if (const AstNodeSel* const selp = VN_CAST(nodep, NodeSel)) {
            return receiverScopePath(selp->fromp());
        } else if (const AstCMethodHard* const callp = VN_CAST(nodep, CMethodHard)) {
            return receiverScopePath(callp->fromp());
        } else if (const AstNodeFTaskRef* const refp = VN_CAST(nodep, NodeFTaskRef)) {
            return refp->dotted();
        }
        return "";
    }
    string receiverPath(const AstNodeExpr* nodep, const AstClass* declaringp) {
        const AstClassRefDType* dtypep = VN_AS(nodep->dtypep()->skipRefp(), ClassRefDType);
        string path = receiverScopePath(nodep);
        if (dtypep->ownerp()) path = VString::dot(path, ".", dtypep->ownerp()->path());
        // Inherited statics belong to the declaring base's independently bound owner.
        while (dtypep->classp() != declaringp) {
            const AstClassExtends* const basep = dtypep->classp()->extendsp();
            UASSERT_OBJ(basep, nodep, "Static member declaring class is not a base");
            dtypep = VN_AS(basep->dtypep()->skipRefp(), ClassRefDType);
            if (dtypep->ownerp()) path = VString::dot(path, ".", dtypep->ownerp()->path());
        }
        return path;
    }
    AstNodeExpr* address(FileLine* fl, AstNodeModule* fromp, const CaptureKey& key,
                         ClassContext* contextp, bool inCtor, AstNodeModule* anchorp = nullptr) {
        if (contextp) {
            const Capture& capture = contextp->captures.at(key);
            UASSERT_OBJ(inCtor || !contextp->object || capture.fieldp, contextp->classp,
                        "Object capture without instance storage");
            return new AstVarRef{fl, inCtor || !contextp->object ? capture.argp : capture.fieldp,
                                 VAccess::READ};
        }
        AstVar* const varp = marker(key.second);
        if (VN_IS(fromp, Package)) {
            // Package initialization has no receiver; use the anchor's concrete instance path.
            string path = key.first;
            if (anchorp && !VN_IS(anchorp, Package)
                && !m_rootScopes.count(path.substr(0, path.find('.'))))
                path = VString::dot(m_instancePaths.at(anchorp), ".", path);
            else if (path.empty())
                path = key.second->name();
            return new AstVarXRef{fl, varp, path, VAccess::READ};
        }
        if (fromp == key.second) { return new AstVarRef{fl, varp, VAccess::READ}; }
        // An enclosing owner is a lexical scope address, not an empty dotted lookup.
        if (key.first.empty()) return new AstScopeAddr{fl, nullptr, varp->dtypep()};
        return new AstVarXRef{fl, varp, key.first, VAccess::READ};
    }
    void addConstructionArgs(AstNew* nodep, AstNodeModule* ownerp, ClassContext* contextp) {
        const AstClassRefDType* const dtypep
            = constructedType(nodep, contextp ? contextp->classp : nullptr);
        const ClassContext& target = m_classes.at(dtypep->classp());
        const bool super = VN_IS(nodep->dtypep()->skipRefp(), VoidDType);
        for (const auto& item : target.captures) {
            const CaptureKey key = constructionKey(item.first, dtypep);
            nodep->addArgsp(new AstArg{
                nodep->fileline(), item.second.argp->name(),
                address(nodep->fileline(), ownerp, key, contextp, super,
                        dtypep->ownerp() ? dtypep->ownerp()->anchorp() : item.second.anchorp)});
        }
    }
    void propagateRequirements() {
        bool changed;
        do {
            changed = false;
            for (ClassContext* const contextp : m_contexts) {
                ClassContext& context = *contextp;
                for (AstNew* const newp : context.constructions) {
                    AstClassRefDType* const dtypep = constructedType(newp, context.classp);
                    const bool super = VN_IS(newp->dtypep()->skipRefp(), VoidDType);
                    for (const auto& item : m_classes.at(dtypep->classp()).captures) {
                        changed |= require(context, constructionKey(item.first, dtypep), !super,
                                           dtypep->ownerp() ? dtypep->ownerp()->anchorp()
                                                            : item.second.anchorp);
                    }
                }
            }
            for (const StaticCall& call : m_staticCalls) {
                const ClassContext& target = m_routines.at(call.refp->taskp());
                ClassContext* const callerp = call.callerp;
                if (callerp) {
                    for (const auto& item : target.captures) {
                        const CaptureKey key{VString::dot(call.path, ".", item.first.first),
                                             item.first.second};
                        changed |= require(*callerp, key, callerp->object, item.second.anchorp);
                    }
                }
            }
        } while (changed);
    }
    void declareCaptures() {
        for (ClassContext* const contextp : m_contexts) {
            ClassContext& context = *contextp;
            unsigned index = 0;
            for (auto& item : context.captures) {
                // Types and sensitivity metadata retain targets even if addresses fold away.
                item.first.second->classCaptureTarget(true);
                Capture& capture = item.second;
                AstNodeDType* const dtypep = marker(item.first.second)->dtypep();
                const string name = "__Vcapture" + cvtToStr(index++) + "p";
                capture.argp = new AstVar{context.classp->fileline(), VVarType::PORT,
                                          name + "_arg", dtypep};
                capture.argp->direction(VDirection::INPUT);
                capture.argp->funcLocal(true);
                context.ctorp->addStmtsp(capture.argp);
                if (context.object && capture.retained) {
                    capture.fieldp
                        = new AstVar{context.classp->fileline(), VVarType::MEMBER, name, dtypep};
                    capture.fieldp->lifetime(VLifetime::AUTOMATIC_IMPLICIT);
                    capture.fieldp->isInternal(true);
                    capture.fieldp->classCaptureArgp(capture.argp);
                    context.classp->addMembersp(capture.fieldp);
                    V3Stats::addStat("Class context, capture fields", 1);
                }
            }
        }
    }
    void materializeReferences() {
        for (ClassContext* const contextp : m_contexts) {
            const ClassContext& context = *contextp;
            for (const auto& item : context.ifaces) {
                AstNodeVarRef* const refp = item.first;
                refp->replaceWith(
                    address(refp->fileline(), context.ownerp, item.second, contextp, false));
                pushDeletep(refp);
            }
            for (const auto& item : context.vars) {
                AstNodeVarRef* const refp = item.first;
                refp->varp()->sensIfacep(item.second.second);
                AstMemberSel* const selp = new AstMemberSel{
                    refp->fileline(),
                    address(refp->fileline(), context.ownerp, item.second, contextp, false),
                    refp->varp()};
                selp->access(refp->access());
                refp->replaceWith(selp);
                pushDeletep(refp);
            }
            for (const auto& item : context.calls) {
                AstNodeFTaskRef* const refp = item.first;
                AstArg* const argsp
                    = refp->argsp() ? refp->argsp()->unlinkFrBackWithNext() : nullptr;
                AstMethodCall* const callp = new AstMethodCall{
                    refp->fileline(),
                    address(refp->fileline(), context.ownerp, item.second, contextp, false),
                    refp->name(), argsp};
                transferCall(refp, callp, item.second.second);
                refp->replaceWith(callp);
                pushDeletep(refp);
            }
            for (AstNew* const newp : context.constructions)
                addConstructionArgs(newp, context.ownerp, contextp);
        }
        for (const auto& item : m_constructions)
            addConstructionArgs(item.first, item.second, nullptr);
        for (const auto& item : m_externalCalls) {
            AstNodeFTaskRef* const refp = item.first;
            AstNodeModule* const targetp = m_declarations.at(refp->taskp());
            AstArg* const argsp = refp->argsp() ? refp->argsp()->unlinkFrBackWithNext() : nullptr;
            AstMethodCall* const callp = new AstMethodCall{
                refp->fileline(),
                address(refp->fileline(), item.second, {refp->dotted(), targetp}, nullptr, false),
                refp->name(), argsp};
            transferCall(refp, callp, targetp);
            refp->replaceWith(callp);
            pushDeletep(refp);
        }
        for (const auto& item : m_staticVariables) {
            AstNodeVarRef* const refp = item.first;
            AstClassPackage* const packagep = VN_AS(m_declarations.at(refp->varp()), ClassPackage);
            const ClassContext& context = m_classes.at(packagep->classp());
            refp->varp()->sensIfacep(packagep);
            AstMemberSel* const selp
                = new AstMemberSel{refp->fileline(),
                                   address(refp->fileline(), item.second,
                                           {context.storagePath, packagep}, nullptr, false),
                                   refp->varp()};
            selp->access(refp->access());
            refp->replaceWith(selp);
            pushDeletep(refp);
        }
    }
    void materializeStaticCalls() {
        for (const StaticCall& call : m_staticCalls) {
            AstNodeFTaskRef* const refp = call.refp;
            const ClassContext& target = m_routines.at(refp->taskp());
            AstMethodCall* const methodp = VN_CAST(refp, MethodCall);
            AstNode* receiverStmtsp = nullptr;
            if (methodp && !methodp->fromp()->isPure()) {
                // Evaluate the source receiver once, without dereferencing its result.
                AstNodeExpr* const exprp = methodp->fromp()->unlinkFrBack();
                receiverStmtsp = newReceiverStmt(exprp);
            }
            for (const auto& item : target.captures) {
                const CaptureKey key{VString::dot(call.path, ".", item.first.first),
                                     item.first.second};
                AstNodeExpr* const exprp = address(refp->fileline(), call.ownerp, key,
                                                   call.callerp, false, item.second.anchorp);
                refp->addArgsp(new AstArg{refp->fileline(), item.second.argp->name(), exprp});
            }
            if (methodp) {
                AstArg* const argsp
                    = refp->argsp() ? refp->argsp()->unlinkFrBackWithNext() : nullptr;
                AstNodeFTaskRef* const callp
                    = VN_IS(refp->taskp(), Task)
                          ? static_cast<AstNodeFTaskRef*>(
                                new AstTaskRef{refp->fileline(), refp->name(), argsp})
                          : static_cast<AstNodeFTaskRef*>(
                                new AstFuncRef{refp->fileline(), refp->name(), argsp});
                transferCall(refp, callp, target.classp);
                if (receiverStmtsp)
                    refp->replaceWith(new AstExprStmt{refp->fileline(), receiverStmtsp, callp});
                else
                    refp->replaceWith(callp);
                pushDeletep(refp);
            }
        }
    }
    void materializeStaticSelections() {
        for (const StaticSelection& selection : m_staticSelections) {
            AstMemberSel* const selp = selection.refp;
            AstNodeExpr* const objectp = selp->fromp()->unlinkFrBack();
            selp->varp()->sensIfacep(selection.key.second);
            AstNodeExpr* storagep = address(selp->fileline(), selection.ownerp, selection.key,
                                            selection.callerp, false);
            if (!objectp->isPure()) {
                storagep = new AstExprStmt{selp->fileline(), newReceiverStmt(objectp), storagep};
            } else {
                pushDeletep(objectp);
            }
            selp->fromp(storagep);
        }
    }
    void visit(AstNodeModule* nodep) override {
        VL_RESTORER(m_modp);
        VL_RESTORER(m_ownerp);
        VL_RESTORER(m_classp);
        VL_RESTORER(m_contextp);
        m_modp = nodep;
        m_ownerp = nodep;
        m_classp = nullptr;
        m_contextp = nullptr;
        iterateChildren(nodep);
    }
    void visit(AstClass* nodep) override {
        VL_RESTORER(m_modp);
        VL_RESTORER(m_classp);
        VL_RESTORER(m_contextp);
        m_modp = nodep;
        m_classp = nodep;
        if (m_collect) {
            m_classes[nodep].ownerp = m_ownerp;
            m_classes[nodep].classp = nodep;
            m_classps.push_back(nodep);
            m_contexts.push_back(&m_classes.at(nodep));
        }
        m_contextp = &m_classes.at(nodep);
        iterateChildren(nodep);
    }
    void visit(AstVar* nodep) override {
        if (m_collect) m_declarations[nodep] = m_modp;
        iterateChildren(nodep);
    }
    void visit(AstNodeProcedure* nodep) override {
        VL_RESTORER(m_contextp);
        // Static processes, including package-class initializers, have no object receiver.
        if (!VN_IS(nodep, InitialAutomatic)) m_contextp = nullptr;
        if (m_collect && m_classp && !VN_IS(nodep, InitialAutomatic)) {
            m_classes.at(m_classp).processes.push_back(nodep);
        }
        iterateChildren(nodep);
    }
    void visit(AstNodeFTask* nodep) override {
        VL_RESTORER(m_contextp);
        if (m_collect) {
            m_declarations[nodep] = m_modp;
            if (m_classp && nodep->isConstructor()) m_classes.at(m_classp).ctorp = nodep;
            if (m_classp && (nodep->isStatic() || !nodep->classMethod())
                && !nodep->isConstructor()) {
                ClassContext& context = m_routines[nodep];
                context.classp = m_classp;
                context.ownerp = m_ownerp;
                context.ctorp = nodep;
                context.object = false;
                m_contexts.push_back(&context);
            }
        }
        const auto it = m_routines.find(nodep);
        if (it != m_routines.end()) m_contextp = &it->second;
        iterateChildren(nodep);
    }
    void visit(AstNodeVarRef* nodep) override {
        if (m_collect || nodep->varp()->isFuncLocal() || nodep->varp()->isParam()) return;
        AstNodeModule* const targetp = m_declarations.at(nodep->varp());
        if (targetp == m_modp) return;
        if (!m_contextp) {
            if (VN_IS(targetp, ClassPackage)) m_staticVariables.emplace_back(nodep, m_ownerp);
            return;
        }
        if (VN_IS(targetp, Class) || VN_IS(targetp, Package)) return;
        // A package class has no implicit module instance to capture. Keep its
        // qualified references and unsupported unqualified references for linking.
        if (!V3ClassContext::instanceOwnerp(m_contextp->classp)) return;
        const AstVarXRef* const xrefp = VN_CAST(nodep, VarXRef);
        string path = xrefp ? xrefp->dotted() : "";
        if (const AstIfaceRefDType* const dtypep
            = VN_CAST(nodep->dtypep()->skipRefp(), IfaceRefDType)) {
            if (!dtypep->isVirtual()) {
                const CaptureKey key{VString::dot(path, ".", dtypep->cellName()),
                                     dtypep->ifaceViaCellp()};
                require(*m_contextp, key, true);
                m_contextp->ifaces.emplace_back(nodep, key);
                return;
            }
        }
        if (const AstClassPackage* const packagep = VN_CAST(targetp, ClassPackage)) {
            path = m_classes.at(packagep->classp()).storagePath;
        }
        const CaptureKey key = dependency(targetp, path);
        ClassContext& context = *m_contextp;
        require(context, key, true);
        context.vars.emplace_back(nodep, key);
    }
    void visit(AstNodeFTaskRef* nodep) override {
        iterateChildren(nodep);
        if (m_collect) return;
        // DPI imports use a loose ABI wrapper, not a method on model storage.
        if (nodep->taskp()->dpiImport()) return;
        if (m_routines.count(nodep->taskp())) {
            const AstMethodCall* const methodp = VN_CAST(nodep, MethodCall);
            const string path
                = methodp ? receiverPath(methodp->fromp(), m_routines.at(nodep->taskp()).classp)
                          : "";
            m_staticCalls.push_back({nodep, m_contextp, m_ownerp, path});
            return;
        }
        if (VN_IS(nodep, MethodCall)) return;
        AstNodeModule* const targetp = m_declarations.at(nodep->taskp());
        if (VN_IS(targetp, Class) || VN_IS(targetp, Package)) return;
        if (!m_contextp) {
            if (VN_IS(m_modp, ClassPackage) && targetp != m_modp) {
                m_externalCalls.emplace_back(nodep, m_ownerp);
            }
            return;
        }
        const CaptureKey key = dependency(targetp, nodep->dotted());
        if (!V3ClassContext::instanceOwnerp(m_contextp->classp)) return;
        ClassContext& context = *m_contextp;
        require(context, key, true);
        context.calls.emplace_back(nodep, key);
    }
    void visit(AstNew* nodep) override {
        iterateChildren(nodep);
        if (m_collect) return;
        if (!nodep->isScoped()) {
            if (AstClassRefDType* const dtypep
                = VN_CAST(nodep->dtypep()->skipRefp(), ClassRefDType)) {
                if (!dtypep->ownerp()) {
                    if (const AstAssign* const assignp = VN_CAST(nodep->backp(), Assign)) {
                        const AstNodeExpr* destp = assignp->lhsp();
                        while (const AstMemberSel* const selp = VN_CAST(destp, MemberSel))
                            destp = selp->fromp();
                        if (const AstVarXRef* const xrefp = VN_CAST(destp, VarXRef)) {
                            if (!xrefp->dotted().empty()
                                && m_classes.at(dtypep->classp()).ownerp != m_ownerp) {
                                AstClassRefDType* const clonep = dtypep->cloneTree(false);
                                clonep->ownerp(new AstClassContext{nodep->fileline(), m_ownerp,
                                                                   xrefp->dotted()});
                                m_rootp->typeTablep()->addTypesp(clonep);
                                nodep->dtypep(clonep);
                            }
                        }
                    }
                }
            }
        }
        if (m_contextp)
            m_contextp->constructions.push_back(nodep);
        else
            m_constructions.emplace_back(nodep, m_ownerp);
    }
    void visit(AstMemberSel* nodep) override {
        iterateChildren(nodep);
        if (!m_collect && nodep->varp() && nodep->varp()->lifetime().isStatic()
            && !nodep->varp()->isParam()
            && VN_IS(m_declarations.at(nodep->varp()), ClassPackage)) {
            const AstClassPackage* const packagep
                = VN_AS(m_declarations.at(nodep->varp()), ClassPackage);
            ClassContext& context = m_classes.at(packagep->classp());
            const CaptureKey key{VString::dot(receiverPath(nodep->fromp(), context.classp), ".",
                                              context.storagePath),
                                 context.packagep};
            m_staticSelections.push_back({nodep, m_contextp, m_ownerp, key});
            if (m_contextp) require(*m_contextp, key, true);
        }
    }
    void visit(AstClassContext*) override {}
    void visit(AstNode* nodep) override { iterateChildren(nodep); }

public:
    explicit ContextPrepareVisitor(AstNetlist* nodep)
        : m_rootp{nodep} {
        // Explicitly rooted paths must not receive a lexical-owner prefix.
        m_rootScopes.insert("$root");
        m_rootScopes.insert(nodep->topModulep()->name());
        for (AstNode* stmtp = nodep->topModulep()->stmtsp(); stmtp; stmtp = stmtp->nextp()) {
            if (const AstCell* const cellp = VN_CAST(stmtp, Cell))
                m_rootScopes.insert(cellp->name());
        }
        iterate(nodep);
        prepareStorage();
        m_collect = false;
        iterate(nodep);
        propagateRequirements();
        declareCaptures();
        indexInstancePaths(nodep->topModulep(), "");
        materializeReferences();
        materializeStaticCalls();
        materializeStaticSelections();
        nodep->foreach([](AstInitialStatic* initialp) {
            initialp->foreach([](AstVarRef* refp) {
                UASSERT_OBJ(!refp->varp()->classCaptureArgp(), refp,
                            "Static initialization references an object capture");
            });
        });
        // Owner paths have now been transferred to construction expressions.
        nodep->foreach(
            [this](AstClassContext* contextp) { pushDeletep(contextp->unlinkFrBack()); });
    }
};

class ContextBindVisitor final : public VNVisitor {
    AstScope* m_scopep = nullptr;  // Concrete construction or initializer scope
    void visit(AstScope* nodep) override {
        VL_RESTORER(m_scopep);
        m_scopep = nodep;
        iterateChildren(nodep);
    }
    void visit(AstScopeAddr* nodep) override {
        if (!nodep->scopep()) {
            const AstNodeModule* const targetp = VN_AS(nodep->dtypep(), ModuleRefDType)->modp();
            AstScope* scopep = m_scopep;
            while (scopep && scopep->modp() != targetp) scopep = scopep->aboveScopep();
            UASSERT_OBJ(scopep, nodep, "Enclosing owner scope not found");
            nodep->scopep(scopep);
        }
        nodep->scopep()->modp()->classCaptureTarget(true);
    }
    void visit(AstVarRef* nodep) override {
        if (!nodep->varp()->classContextMarker()) return;
        AstScope* const scopep = nodep->varScopep()->scopep();
        scopep->modp()->classCaptureTarget(true);
        nodep->replaceWith(new AstScopeAddr{nodep->fileline(), scopep, nodep->dtypep()});
        pushDeletep(nodep);
    }
    void visit(AstVarScope* nodep) override {
        if (nodep->varp()->classContextMarker()) pushDeletep(nodep->unlinkFrBack());
    }
    void visit(AstVar* nodep) override {
        if (nodep->classContextMarker())
            pushDeletep(nodep->unlinkFrBack());
        else
            iterateChildren(nodep);
    }
    void visit(AstNode* nodep) override { iterateChildren(nodep); }

public:
    explicit ContextBindVisitor(AstNetlist* nodep) { iterate(nodep); }
};
}  //namespace

AstNodeModule* V3ClassContext::instanceOwnerp(const AstClass* nodep) {
    for (AstNode* parentp = nodep->aboveLoopp(); parentp; parentp = parentp->aboveLoopp()) {
        if (AstNodeModule* const modp = VN_CAST(parentp, NodeModule)) {
            if (!VN_IS(modp, Class)) return VN_IS(modp, Package) ? nullptr : modp;
        }
    }
    return nullptr;
}

void V3ClassContext::prepare(AstNetlist* nodep) {
    { ContextPrepareVisitor{nodep}; }
    V3Global::dumpCheckGlobalTree("classcontext_prepare", 0, dumpTreeEitherLevel() >= 3);
}
void V3ClassContext::bind(AstNetlist* nodep) {
    { ContextBindVisitor{nodep}; }
    V3Global::dumpCheckGlobalTree("classcontext_bind", 0, dumpTreeEitherLevel() >= 3);
}
