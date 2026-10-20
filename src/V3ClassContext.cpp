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
// Before scoping, turn external class dependencies into explicit receivers.
// Scope-address placeholders travel through ordinary scoped linking, including
// interface aliases. After linking, replace them with typed scope addresses and
// prohibit inlining their target definitions. Thus method bodies contain only
// object-relative captures; concrete instance addresses occur at construction.
// Captures are non-owning: module storage lives for the entire model lifetime.
// Constructor dependencies propagate until stable, including nested and base
// construction. Base-only inputs need no field in the derived subobject.
// Static routines keep their lexical declarations and receive their own hidden
// inputs; no object receiver exists in their bodies. Only mutable instance-owned
// static fields move before scoping, through V3Class's single package factory.
//*************************************************************************

#include "V3PchAstNoMT.h"

#include "V3ClassContext.h"

#include "V3Class.h"
#include "V3Stats.h"

VL_DEFINE_DEBUG_FUNCTIONS;

namespace {
using CaptureKey = std::pair<string, AstNodeModule*>;
struct Capture final {
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
    std::vector<std::pair<AstNodeFTaskRef*, CaptureKey>> calls;
    std::vector<AstNew*> constructions;
};
struct StaticCall final {
    AstNodeFTaskRef* refp;
    ClassContext* callerp;
    AstNodeModule* ownerp;
};

class ContextPrepareVisitor final : public VNVisitor {
    AstNetlist* const m_rootp;
    AstNodeModule* m_modp = nullptr;
    AstNodeModule* m_ownerp = nullptr;
    AstClass* m_classp = nullptr;
    ClassContext* m_contextp = nullptr;
    std::map<AstNode*, AstNodeModule*> m_declarations;
    std::map<AstClass*, ClassContext> m_classes;
    std::vector<AstClass*> m_classps;
    std::map<AstNodeFTask*, ClassContext> m_routines;
    std::vector<ClassContext*> m_contexts;
    std::map<AstNodeModule*, AstVar*> m_markers;
    std::vector<std::pair<AstNew*, AstNodeModule*>> m_constructions;
    std::vector<AstMemberSel*> m_staticSelections;
    std::vector<std::pair<AstNodeVarRef*, AstNodeModule*>> m_staticVariables;
    std::vector<StaticCall> m_staticCalls;
    std::vector<std::pair<AstNodeFTaskRef*, AstNodeModule*>> m_externalCalls;
    bool m_collect = true;
    unsigned m_receiverIndex = 0;  // Unique expression-local static-call receivers

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
            for (AstNode* nodep = classp->stmtsp(); nodep; nodep = nodep->nextp()) {
                if (const AstVar* const varp = VN_CAST(nodep, Var)) {
                    if (varp->lifetime().isStatic() && !varp->isParam()) moves.push_back(nodep);
                } else if (VN_IS(nodep, InitialStatic) || VN_IS(nodep, Initial)) {
                    moves.push_back(nodep);
                }
            }
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
    bool require(ClassContext& context, const CaptureKey& key, bool retained) {
        const auto it = context.captures.emplace(key, Capture{});
        const bool changed = it.second || (retained && !it.first->second.retained);
        it.first->second.retained |= retained;
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
    AstNodeExpr* address(FileLine* fl, AstNodeModule* fromp, const CaptureKey& key,
                         ClassContext* contextp, bool inCtor) {
        if (contextp) {
            const Capture& capture = contextp->captures.at(key);
            return new AstVarRef{fl, inCtor || !contextp->object ? capture.argp : capture.fieldp,
                                 VAccess::READ};
        }
        AstVar* const varp = marker(key.second);
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
            nodep->addArgsp(new AstArg{nodep->fileline(), item.second.argp->name(),
                                       address(nodep->fileline(), ownerp, key, contextp, super)});
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
                        changed |= require(context, constructionKey(item.first, dtypep), !super);
                    }
                }
            }
            for (const StaticCall& call : m_staticCalls) {
                const ClassContext& target = m_routines.at(call.refp->taskp());
                ClassContext* callerp = call.callerp;
                if (VN_IS(call.refp, MethodCall)) callerp = &m_classes.at(target.classp);
                if (callerp) {
                    for (const auto& item : target.captures) {
                        changed |= require(*callerp, item.first, callerp->object);
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
                    context.classp->addStmtsp(capture.fieldp);
                    V3Stats::addStat("Class context, capture fields", 1);
                }
            }
        }
    }
    void materializeReferences() {
        for (ClassContext* const contextp : m_contexts) {
            const ClassContext& context = *contextp;
            for (const auto& item : context.vars) {
                AstNodeVarRef* const refp = item.first;
                refp->varp()->sensStoragep(item.second.second);
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
            refp->varp()->sensStoragep(packagep);
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
                // Hidden arguments must not duplicate side effects in the source receiver.
                AstNodeExpr* const exprp = methodp->fromp()->unlinkFrBack();
                AstVar* const receiverp = new AstVar{
                    refp->fileline(), VVarType::BLOCKTEMP,
                    "__VstaticReceiver" + cvtToStr(m_receiverIndex++), exprp->dtypep()};
                receiverp->funcLocal(true);
                receiverp->lifetime(VLifetime::AUTOMATIC_IMPLICIT);
                receiverStmtsp = receiverp;
                receiverStmtsp->addNext(new AstAssign{
                    refp->fileline(), new AstVarRef{refp->fileline(), receiverp, VAccess::WRITE},
                    exprp});
                methodp->fromp(new AstVarRef{refp->fileline(), receiverp, VAccess::READ});
            }
            for (const auto& item : target.captures) {
                AstNodeExpr* exprp;
                if (methodp) {
                    const Capture& capture = m_classes.at(target.classp).captures.at(item.first);
                    AstMemberSel* const selp = new AstMemberSel{
                        refp->fileline(), methodp->fromp()->cloneTree(false), capture.fieldp};
                    selp->access(VAccess::READ);
                    exprp = selp;
                } else {
                    exprp
                        = address(refp->fileline(), call.ownerp, item.first, call.callerp, false);
                }
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
        for (AstMemberSel* const selp : m_staticSelections) {
            AstClass* const declaringp
                = VN_AS(m_declarations.at(selp->varp()), ClassPackage)->classp();
            const ClassContext& context = m_classes.at(declaringp);
            const Capture& capture = context.captures.at({context.storagePath, context.packagep});
            AstNodeExpr* const objectp = selp->fromp()->unlinkFrBack();
            selp->varp()->sensStoragep(context.packagep);
            AstMemberSel* const storagep
                = new AstMemberSel{selp->fileline(), objectp, capture.fieldp};
            storagep->access(VAccess::READ);
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
            m_staticCalls.push_back({nodep, m_contextp, m_ownerp});
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
            m_staticSelections.push_back(nodep);
            const AstClassPackage* const packagep
                = VN_AS(m_declarations.at(nodep->varp()), ClassPackage);
            ClassContext& context = m_classes.at(packagep->classp());
            require(context, {context.storagePath, context.packagep}, true);
        }
    }
    void visit(AstClassContext*) override {}
    void visit(AstNode* nodep) override { iterateChildren(nodep); }

public:
    explicit ContextPrepareVisitor(AstNetlist* nodep)
        : m_rootp{nodep} {
        iterate(nodep);
        prepareStorage();
        m_collect = false;
        iterate(nodep);
        propagateRequirements();
        declareCaptures();
        materializeReferences();
        materializeStaticCalls();
        materializeStaticSelections();
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
