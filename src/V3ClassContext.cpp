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
//*************************************************************************

#include "V3PchAstNoMT.h"

#include "V3ClassContext.h"

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
    AstNodeModule* ownerp = nullptr;
    AstNodeFTask* ctorp = nullptr;
    AstClassPackage* packagep = nullptr;
    string storagePath;
    std::map<CaptureKey, Capture> captures;
    std::vector<std::pair<AstNodeVarRef*, CaptureKey>> vars;
    std::vector<std::pair<AstNodeFTaskRef*, CaptureKey>> calls;
    std::vector<AstNew*> constructions;
};

class ContextPrepareVisitor final : public VNVisitor {
    AstNetlist* const m_rootp;
    AstNodeModule* m_modp = nullptr;
    AstNodeModule* m_ownerp = nullptr;
    AstClass* m_classp = nullptr;
    std::map<AstNode*, AstNodeModule*> m_declarations;
    std::map<AstClass*, ClassContext> m_classes;
    std::vector<AstClass*> m_classps;
    std::map<AstNodeModule*, AstVar*> m_markers;
    std::vector<std::pair<AstNew*, AstNodeModule*>> m_constructions;
    std::vector<AstMemberSel*> m_staticSelections;
    std::vector<std::pair<AstNodeVarRef*, AstNodeModule*>> m_staticVariables;
    std::map<AstNodeFTask*, AstClass*> m_staticFunctions;
    std::vector<std::pair<AstNodeFTaskRef*, AstNodeModule*>> m_staticCalls;
    bool m_collect = true;

    void prepareStorage() {
        unsigned index = 0;
        for (AstClass* const classp : m_classps) {
            ClassContext& context = m_classes.at(classp);
            std::vector<AstNode*> moves;
            std::vector<AstNodeFTask*> staticFunctions;
            for (AstNode* nodep = classp->stmtsp(); nodep; nodep = nodep->nextp()) {
                if (const AstVar* const varp = VN_CAST(nodep, Var)) {
                    if (varp->lifetime().isStatic() && !varp->isParam()) moves.push_back(nodep);
                } else if (VN_IS(nodep, InitialStatic) || VN_IS(nodep, Initial)) {
                    moves.push_back(nodep);
                } else if (AstNodeFTask* const taskp = VN_CAST(nodep, NodeFTask)) {
                    if (taskp->isStatic()) staticFunctions.push_back(taskp);
                }
            }
            bool needsStorage = !moves.empty();
            for (AstNodeFTask* const taskp : staticFunctions) {
                needsStorage |= taskp->exists([&](const AstNodeVarRef* refp) {
                    AstNodeModule* const ownerp = m_declarations.at(refp->varp());
                    return ownerp != classp && !VN_IS(ownerp, Class) && !VN_IS(ownerp, Package);
                });
                needsStorage |= taskp->exists([](const AstNew*) { return true; });
            }
            if (needsStorage) {
                for (AstNodeFTask* const taskp : staticFunctions) {
                    moves.push_back(taskp);
                    m_staticFunctions.emplace(taskp, classp);
                    taskp->classMethod(false);
                    taskp->isStatic(false);
                }
            }
            if (moves.empty()) continue;
            AstClassPackage* const packagep = new AstClassPackage{
                classp->fileline(), classp->name() + "__Vclpkg", classp->libname()};
            packagep->classp(classp);
            packagep->classCaptureTarget(true);
            packagep->timeunit(classp->timeunit());
            classp->classOrPackagep(packagep);
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
                movep->foreach([](AstVarRef* refp) { refp->classOrPackagep(nullptr); });
            }
            require(context, {context.storagePath, packagep}, true);
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
                         AstClass* classp, bool inCtor) {
        if (classp) {
            const Capture& capture = m_classes.at(classp).captures.at(key);
            return new AstVarRef{fl, inCtor ? capture.argp : capture.fieldp, VAccess::READ};
        }
        AstVar* const varp = marker(key.second);
        if (fromp == key.second) { return new AstVarRef{fl, varp, VAccess::READ}; }
        return new AstVarXRef{fl, varp, key.first, VAccess::READ};
    }
    void addConstructionArgs(AstNew* nodep, AstNodeModule* ownerp, AstClass* classp) {
        const AstClassRefDType* const dtypep = constructedType(nodep, classp);
        const ClassContext& target = m_classes.at(dtypep->classp());
        const bool super = VN_IS(nodep->dtypep()->skipRefp(), VoidDType);
        for (const auto& item : target.captures) {
            const CaptureKey key = constructionKey(item.first, dtypep);
            nodep->addArgsp(new AstArg{nodep->fileline(), item.second.argp->name(),
                                       address(nodep->fileline(), ownerp, key, classp, super)});
        }
    }
    void materialize() {
        bool changed;
        do {
            changed = false;
            for (AstClass* const classp : m_classps) {
                ClassContext& context = m_classes.at(classp);
                for (AstNew* const newp : context.constructions) {
                    AstClassRefDType* const dtypep = constructedType(newp, classp);
                    const bool super = VN_IS(newp->dtypep()->skipRefp(), VoidDType);
                    for (const auto& item : m_classes.at(dtypep->classp()).captures) {
                        changed |= require(context, constructionKey(item.first, dtypep), !super);
                    }
                }
            }
        } while (changed);
        for (AstClass* const classp : m_classps) {
            ClassContext& context = m_classes.at(classp);
            unsigned index = 0;
            for (auto& item : context.captures) {
                Capture& capture = item.second;
                AstNodeDType* const dtypep = marker(item.first.second)->dtypep();
                const string name = "__Vcapture" + cvtToStr(index++) + "p";
                capture.argp
                    = new AstVar{classp->fileline(), VVarType::PORT, name + "_arg", dtypep};
                capture.argp->direction(VDirection::INPUT);
                capture.argp->funcLocal(true);
                context.ctorp->addStmtsp(capture.argp);
                if (capture.retained) {
                    capture.fieldp
                        = new AstVar{classp->fileline(), VVarType::MEMBER, name, dtypep};
                    capture.fieldp->lifetime(VLifetime::AUTOMATIC_IMPLICIT);
                    capture.fieldp->classCaptureArgp(capture.argp);
                    classp->addStmtsp(capture.fieldp);
                    V3Stats::addStat("Class context, capture fields", 1);
                }
            }
        }
        for (AstClass* const classp : m_classps) {
            const ClassContext& context = m_classes.at(classp);
            for (const auto& item : context.vars) {
                AstNodeVarRef* const refp = item.first;
                refp->varp()->sensStoragep(item.second.second);
                AstMemberSel* const selp = new AstMemberSel{
                    refp->fileline(),
                    address(refp->fileline(), context.ownerp, item.second, classp, false),
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
                    address(refp->fileline(), context.ownerp, item.second, classp, false),
                    refp->name(), argsp};
                callp->taskp(refp->taskp());
                callp->classOrPackagep(item.second.second);
                callp->dtypeFrom(refp);
                refp->replaceWith(callp);
                pushDeletep(refp);
            }
            for (AstNew* const newp : context.constructions)
                addConstructionArgs(newp, context.ownerp, classp);
        }
        for (const auto& item : m_constructions)
            addConstructionArgs(item.first, item.second, nullptr);
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
        for (const auto& item : m_staticCalls) {
            AstNodeFTaskRef* const refp = item.first;
            const ClassContext& context = m_classes.at(m_staticFunctions.at(refp->taskp()));
            AstNodeExpr* receiverp;
            if (AstMethodCall* const methodp = VN_CAST(refp, MethodCall)) {
                const Capture& capture = m_classes.at(context.packagep->classp())
                                             .captures.at({context.storagePath, context.packagep});
                receiverp = new AstMemberSel{refp->fileline(), methodp->fromp()->unlinkFrBack(),
                                             capture.fieldp};
                VN_AS(receiverp, MemberSel)->access(VAccess::READ);
            } else {
                receiverp = address(refp->fileline(), item.second,
                                    {context.storagePath, context.packagep}, nullptr, false);
            }
            AstArg* const argsp = refp->argsp() ? refp->argsp()->unlinkFrBackWithNext() : nullptr;
            AstMethodCall* const callp
                = new AstMethodCall{refp->fileline(), receiverp, refp->name(), argsp};
            callp->taskp(refp->taskp());
            callp->classOrPackagep(context.packagep);
            callp->dtypeFrom(refp);
            refp->replaceWith(callp);
            pushDeletep(refp);
        }
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
        m_modp = nodep;
        m_ownerp = nodep;
        m_classp = nullptr;
        iterateChildren(nodep);
    }
    void visit(AstClass* nodep) override {
        VL_RESTORER(m_modp);
        VL_RESTORER(m_classp);
        m_modp = nodep;
        m_classp = nodep;
        if (m_collect) {
            m_classes[nodep].ownerp = m_ownerp;
            m_classps.push_back(nodep);
        }
        iterateChildren(nodep);
    }
    void visit(AstVar* nodep) override {
        if (m_collect) m_declarations[nodep] = m_modp;
        iterateChildren(nodep);
    }
    void visit(AstNodeFTask* nodep) override {
        if (m_collect) {
            m_declarations[nodep] = m_modp;
            if (m_classp && nodep->isConstructor()) m_classes.at(m_classp).ctorp = nodep;
        }
        iterateChildren(nodep);
    }
    void visit(AstNodeVarRef* nodep) override {
        if (m_collect || nodep->varp()->isFuncLocal() || nodep->varp()->isParam()) return;
        AstNodeModule* const targetp = m_declarations.at(nodep->varp());
        if (targetp == m_modp) return;
        if (!m_classp) {
            if (VN_IS(targetp, ClassPackage)) m_staticVariables.emplace_back(nodep, m_ownerp);
            return;
        }
        if (VN_IS(targetp, Class) || VN_IS(targetp, Package)) return;
        const AstVarXRef* const xrefp = VN_CAST(nodep, VarXRef);
        string path = xrefp ? xrefp->dotted() : "";
        if (const AstClassPackage* const packagep = VN_CAST(targetp, ClassPackage)) {
            path = m_classes.at(packagep->classp()).storagePath;
        }
        const CaptureKey key = dependency(targetp, path);
        ClassContext& context = m_classes.at(m_classp);
        require(context, key, true);
        context.vars.emplace_back(nodep, key);
    }
    void visit(AstNodeFTaskRef* nodep) override {
        iterateChildren(nodep);
        if (m_collect) return;
        if (m_staticFunctions.count(nodep->taskp())) {
            if (m_classp && !VN_IS(nodep, MethodCall)) {
                const ClassContext& target = m_classes.at(m_staticFunctions.at(nodep->taskp()));
                const CaptureKey key{target.storagePath, target.packagep};
                ClassContext& context = m_classes.at(m_classp);
                require(context, key, true);
                context.calls.emplace_back(nodep, key);
                return;
            }
            m_staticCalls.emplace_back(nodep, m_ownerp);
            return;
        }
        if (VN_IS(nodep, MethodCall) && nodep->taskp()->isStatic()) {
            // Pure static methods need no owner; retain the existing static-call lowering.
            AstArg* const argsp
                = nodep->argsp() ? nodep->argsp()->unlinkFrBackWithNext() : nullptr;
            AstNodeFTaskRef* const callp
                = VN_IS(nodep->taskp(), Task)
                      ? static_cast<AstNodeFTaskRef*>(
                            new AstTaskRef{nodep->fileline(), nodep->name(), argsp})
                      : static_cast<AstNodeFTaskRef*>(
                            new AstFuncRef{nodep->fileline(), nodep->name(), argsp});
            callp->taskp(nodep->taskp());
            callp->classOrPackagep(nodep->classOrPackagep());
            callp->dtypeFrom(nodep);
            nodep->replaceWith(callp);
            pushDeletep(nodep);
            return;
        }
        if (!m_classp || VN_IS(nodep, MethodCall)) return;
        AstNodeModule* const targetp = m_declarations.at(nodep->taskp());
        if (VN_IS(targetp, Class) || VN_IS(targetp, Package)) return;
        const CaptureKey key = dependency(targetp, nodep->dotted());
        ClassContext& context = m_classes.at(m_classp);
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
        if (m_classp)
            m_classes.at(m_classp).constructions.push_back(nodep);
        else
            m_constructions.emplace_back(nodep, m_ownerp);
    }
    void visit(AstMemberSel* nodep) override {
        iterateChildren(nodep);
        if (!m_collect && nodep->varp() && nodep->varp()->lifetime().isStatic()
            && !nodep->varp()->isParam()
            && VN_IS(m_declarations.at(nodep->varp()), ClassPackage)) {
            m_staticSelections.push_back(nodep);
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
        materialize();
        // Owner paths have now been transferred to construction expressions.
        nodep->foreach(
            [this](AstClassContext* contextp) { pushDeletep(contextp->unlinkFrBack()); });
    }
};

class ContextBindVisitor final : public VNVisitor {
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

void V3ClassContext::prepare(AstNetlist* nodep) {
    { ContextPrepareVisitor{nodep}; }
    V3Global::dumpCheckGlobalTree("classcontext_prepare", 0, dumpTreeEitherLevel() >= 3);
}
void V3ClassContext::bind(AstNetlist* nodep) {
    { ContextBindVisitor{nodep}; }
    V3Global::dumpCheckGlobalTree("classcontext_bind", 0, dumpTreeEitherLevel() >= 3);
}
