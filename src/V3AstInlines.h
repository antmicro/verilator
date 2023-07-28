// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Ast node inline functions
//
// Code available from: https://verilator.org
//
//*************************************************************************
//
// Copyright 2003-2023 by Wilson Snyder. This program is free software; you
// can redistribute it and/or modify it under the terms of either the GNU
// Lesser General Public License Version 3 or the Perl Artistic License
// Version 2.0.
// SPDX-License-Identifier: LGPL-3.0-only OR Artistic-2.0
//
//*************************************************************************

#ifndef VERILATOR_V3ASTINLINES_H_
#define VERILATOR_V3ASTINLINES_H_

#ifndef VERILATOR_V3AST_H_
#error "Use V3Ast.h as the include"
#include "V3Ast.h"  // This helps code analysis tools pick up symbols in V3Ast.h and related
#endif

//######################################################################
// Inline METHODS

int AstNode::width() const VL_MT_STABLE { return dtypep() ? dtypep()->width() : 0; }
int AstNode::widthMin() const { return dtypep() ? dtypep()->widthMin() : 0; }
bool AstNode::width1() const {  // V3Const uses to know it can optimize
    return dtypep() && dtypep()->width() == 1;
}
int AstNode::widthInstrs() const {
    return (!dtypep() ? 1 : (dtypep()->isWide() ? dtypep()->widthWords() : 1));
}
bool AstNode::isDouble() const VL_MT_STABLE {
    return dtypep() && VN_IS(dtypep(), BasicDType) && VN_AS(dtypep(), BasicDType)->isDouble();
}
bool AstNode::isString() const VL_MT_STABLE {
    return dtypep() && dtypep()->basicp() && dtypep()->basicp()->isString();
}
bool AstNode::isSigned() const VL_MT_STABLE { return dtypep() && dtypep()->isSigned(); }

bool AstNode::isClassHandleValue() const {
    return (VN_IS(this, Const) && VN_AS(this, Const)->num().isNull())
           || VN_IS(dtypep(), ClassRefDType);
}
bool AstNode::isZero() const {
    return (VN_IS(this, Const) && VN_AS(this, Const)->num().isEqZero());
}
bool AstNode::isNeqZero() const {
    return (VN_IS(this, Const) && VN_AS(this, Const)->num().isNeqZero());
}
bool AstNode::isOne() const { return (VN_IS(this, Const) && VN_AS(this, Const)->num().isEqOne()); }
bool AstNode::isAllOnes() const {
    return (VN_IS(this, Const) && VN_AS(this, Const)->isEqAllOnes());
}
bool AstNode::isAllOnesV() const {
    return (VN_IS(this, Const) && VN_AS(this, Const)->isEqAllOnesV());
}
bool AstNode::sameTree(const AstNode* node2p) const {
    return sameTreeIter(this, node2p, true, false);
}
bool AstNode::sameGateTree(const AstNode* node2p) const {
    return sameTreeIter(this, node2p, true, true);
}

int AstNodeArrayDType::left() const VL_MT_STABLE { return rangep()->leftConst(); }
int AstNodeArrayDType::right() const VL_MT_STABLE { return rangep()->rightConst(); }
int AstNodeArrayDType::hi() const VL_MT_STABLE { return rangep()->hiConst(); }
int AstNodeArrayDType::lo() const VL_MT_STABLE { return rangep()->loConst(); }
int AstNodeArrayDType::elementsConst() const VL_MT_STABLE { return rangep()->elementsConst(); }
VNumRange AstNodeArrayDType::declRange() const VL_MT_STABLE { return VNumRange{left(), right()}; }

AstRange::AstRange(FileLine* fl, int left, int right)
    : ASTGEN_SUPER_Range(fl) {
    leftp(new AstConst{fl, static_cast<uint32_t>(left)});
    rightp(new AstConst{fl, static_cast<uint32_t>(right)});
}
AstRange::AstRange(FileLine* fl, const VNumRange& range)
    : ASTGEN_SUPER_Range(fl) {
    leftp(new AstConst{fl, static_cast<uint32_t>(range.left())});
    rightp(new AstConst{fl, static_cast<uint32_t>(range.right())});
}
int AstRange::leftConst() const VL_MT_STABLE {
    AstConst* const constp = VN_CAST(leftp(), Const);
    return (constp ? constp->toSInt() : 0);
}
int AstRange::rightConst() const VL_MT_STABLE {
    AstConst* const constp = VN_CAST(rightp(), Const);
    return (constp ? constp->toSInt() : 0);
}

int AstQueueDType::boundConst() const VL_MT_STABLE {
    AstConst* const constp = VN_CAST(boundp(), Const);
    return (constp ? constp->toSInt() : 0);
}

AstPin::AstPin(FileLine* fl, int pinNum, AstVarRef* varname, AstNode* exprp)
    : ASTGEN_SUPER_Pin(fl)
    , m_pinNum{pinNum}
    , m_name{varname->name()} {
    this->exprp(exprp);
}

AstPackArrayDType::AstPackArrayDType(FileLine* fl, VFlagChildDType, AstNodeDType* dtp,
                                     AstRange* rangep)
    : ASTGEN_SUPER_PackArrayDType(fl) {
    childDTypep(dtp);  // Only for parser
    refDTypep(nullptr);
    this->rangep(rangep);
    dtypep(nullptr);  // V3Width will resolve
    const int width = subDTypep()->width() * rangep->elementsConst();
    widthForce(width, width);
}
AstPackArrayDType::AstPackArrayDType(FileLine* fl, AstNodeDType* dtp, AstRange* rangep)
    : ASTGEN_SUPER_PackArrayDType(fl) {
    refDTypep(dtp);
    this->rangep(rangep);
    dtypep(this);
    const int width = subDTypep()->width() * rangep->elementsConst();
    widthForce(width, width);
}

int AstBasicDType::hi() const { return (rangep() ? rangep()->hiConst() : m.m_nrange.hi()); }
int AstBasicDType::lo() const { return (rangep() ? rangep()->loConst() : m.m_nrange.lo()); }
int AstBasicDType::elements() const {
    return (rangep() ? rangep()->elementsConst() : m.m_nrange.elements());
}
bool AstBasicDType::ascending() const {
    return (rangep() ? rangep()->ascending() : m.m_nrange.ascending());
}

bool AstActive::hasClocked() const { return m_sensesp->hasClocked(); }
bool AstActive::hasCombo() const { return m_sensesp->hasCombo(); }

AstElabDisplay::AstElabDisplay(FileLine* fl, VDisplayType dispType, AstNodeExpr* exprsp)
    : ASTGEN_SUPER_ElabDisplay(fl) {
    addFmtp(new AstSFormatF{fl, AstSFormatF::NoFormat{}, exprsp});
    m_displayType = dispType;
}

AstCStmt::AstCStmt(FileLine* fl, const string& textStmt)
    : ASTGEN_SUPER_CStmt(fl) {
    addExprsp(new AstText{fl, textStmt, true});
}

AstCExpr::AstCExpr(FileLine* fl, const string& textStmt, int setwidth, bool cleanOut)
    : ASTGEN_SUPER_CExpr(fl)
    , m_cleanOut{cleanOut}
    , m_pure{true} {
    addExprsp(new AstText{fl, textStmt, true});
    if (setwidth) dtypeSetLogicSized(setwidth, VSigning::UNSIGNED);
}

AstVarRef::AstVarRef(FileLine* fl, AstVar* varp, const VAccess& access)
    : ASTGEN_SUPER_VarRef(fl, varp->name(), varp, access) {}
// This form only allowed post-link (see above)
AstVarRef::AstVarRef(FileLine* fl, AstVarScope* varscp, const VAccess& access)
    : ASTGEN_SUPER_VarRef(fl, varscp->varp()->name(), varscp->varp(), access) {
    varScopep(varscp);
}
bool AstVarRef::same(const AstVarRef* samep) const {
    if (varScopep()) {
        return (varScopep() == samep->varScopep() && access() == samep->access());
    } else {
        return (selfPointer() == samep->selfPointer() && varp()->name() == samep->varp()->name()
                && access() == samep->access());
    }
}
bool AstVarRef::sameNoLvalue(AstVarRef* samep) const {
    if (varScopep()) {
        return (varScopep() == samep->varScopep());
    } else {
        return (selfPointer() == samep->selfPointer()
                && (!selfPointer().empty() || !samep->selfPointer().empty())
                && varp()->name() == samep->varp()->name());
    }
}

AstVarXRef::AstVarXRef(FileLine* fl, AstVar* varp, const string& dotted, const VAccess& access)
    : ASTGEN_SUPER_VarXRef(fl, varp->name(), varp, access)
    , m_dotted{dotted} {
    dtypeFrom(varp);
}

AstStmtExpr* AstNodeExpr::makeStmt() { return new AstStmtExpr{fileline(), this}; }

//======================================================================
// Iterators

template <typename Visitor>
void AstNode::iterateChildren(Visitor& v) {
    // This is a very hot function
    // Optimization note: Grabbing m_op#p->m_nextp is a net loss
    ASTNODE_PREFETCH(m_op1p);
    ASTNODE_PREFETCH(m_op2p);
    ASTNODE_PREFETCH(m_op3p);
    ASTNODE_PREFETCH(m_op4p);
    if (m_op1p) m_op1p->iterateAndNext(v);
    if (m_op2p) m_op2p->iterateAndNext(v);
    if (m_op3p) m_op3p->iterateAndNext(v);
    if (m_op4p) m_op4p->iterateAndNext(v);
}

template <typename Visitor>
void AstNode::iterateChildrenConst(Visitor& v) {
    // This is a very hot function
    ASTNODE_PREFETCH(m_op1p);
    ASTNODE_PREFETCH(m_op2p);
    ASTNODE_PREFETCH(m_op3p);
    ASTNODE_PREFETCH(m_op4p);
    if (m_op1p) m_op1p->iterateAndNextConst(v);
    if (m_op2p) m_op2p->iterateAndNextConst(v);
    if (m_op3p) m_op3p->iterateAndNextConst(v);
    if (m_op4p) m_op4p->iterateAndNextConst(v);
}

template <typename Visitor>
void AstNode::iterateAndNext(Visitor& v) {
    // This is a very hot function
    // IMPORTANT: If you replace a node that's the target of this iterator,
    // then the NEW node will be iterated on next, it isn't skipped!
    // Future versions of this function may require the node to have a back to be iterated;
    // there's no lower level reason yet though the back must exist.
    AstNode* nodep = this;
#ifdef VL_DEBUG  // Otherwise too hot of a function for debug
    UASSERT_OBJ(!(nodep && !nodep->m_backp), nodep, "iterateAndNext node has no back");
#endif
    // cppcheck-suppress knownConditionTrueFalse
    if (nodep) ASTNODE_PREFETCH(nodep->m_nextp);
    while (nodep) {  // effectively: if (!this) return;  // Callers rely on this
        if (nodep->m_nextp) ASTNODE_PREFETCH(nodep->m_nextp->m_nextp);
        AstNode* niterp = nodep;  // Pointer may get stomped via m_iterpp if the node is edited
        // Desirable check, but many places where multiple iterations are OK
        // UASSERT_OBJ(!niterp->m_iterpp, niterp, "IterateAndNext under iterateAndNext may miss
        // edits"); Optimization note: Doing PREFETCH_RW on m_iterpp is a net even
        // cppcheck-suppress nullPointer
        niterp->m_iterpp = &niterp;
        niterp->accept(v);
        // accept may do a replaceNode and change niterp on us...
        // niterp maybe nullptr, so need cast if printing
        // if (niterp != nodep) UINFO(1,"iterateAndNext edited "<<cvtToHex(nodep)
        //                             <<" now into "<<cvtToHex(niterp)<<endl);
        if (!niterp) return;  // Perhaps node deleted inside accept
        niterp->m_iterpp = nullptr;
        if (VL_UNLIKELY(niterp != nodep)) {  // Edited node inside accept
            nodep = niterp;
        } else {  // Unchanged node (though maybe updated m_next), just continue loop
            nodep = niterp->m_nextp;
        }
    }
}

template <typename Visitor>
void AstNode::iterateListBackwardsConst(Visitor& v) {
    AstNode* nodep = this;
    while (nodep->m_nextp) nodep = nodep->m_nextp;
    while (nodep) {
        // Edits not supported: nodep->m_iterpp = &nodep;
        nodep->accept(v);
        if (nodep->backp()->m_nextp == nodep) {
            nodep = nodep->backp();
        } else {
            nodep = nullptr;
        }  // else: backp points up the tree.
    }
}

template <typename Visitor>
void AstNode::iterateChildrenBackwardsConst(Visitor& v) {
    if (m_op1p) m_op1p->iterateListBackwardsConst(v);
    if (m_op2p) m_op2p->iterateListBackwardsConst(v);
    if (m_op3p) m_op3p->iterateListBackwardsConst(v);
    if (m_op4p) m_op4p->iterateListBackwardsConst(v);
}

template <typename Visitor>
void AstNode::iterateAndNextConst(Visitor& v) {
    // Keep following the current list even if edits change it
    AstNode* nodep = this;
    do {
        AstNode* const nnextp = nodep->m_nextp;
        ASTNODE_PREFETCH(nnextp);
        nodep->accept(v);
        nodep = nnextp;
    } while (nodep);
}

template <typename Visitor>
AstNode* AstNode::iterateSubtreeReturnEdits(Visitor& v) {
    // Some visitors perform tree edits (such as V3Const), and may even
    // replace/delete the exact nodep that the visitor is called with.  If
    // this happens, the parent will lose the handle to the node that was
    // processed.
    // To solve this, this function returns the pointer to the replacement node,
    // which in many cases is just the same node that was passed in.
    AstNode* nodep = this;  // Note "this" may point to bogus point later in this function
    if (VN_IS(nodep, Netlist)) {
        // Calling on top level; we know the netlist won't get replaced
        nodep->accept(v);
    } else if (!nodep->backp()) {
        // Calling on standalone tree; insert a shim node so we can keep
        // track, then delete it on completion
        AstBegin* const tempp = new AstBegin{nodep->fileline(), "[EditWrapper]", nodep};
        {
            VL_DO_DANGLING(tempp->stmtsp()->accept(v),
                           nodep);  // nodep to null as may be replaced
        }
        nodep = tempp->stmtsp()->unlinkFrBackWithNext();
        VL_DO_DANGLING(tempp->deleteTree(), tempp);
    } else {
        // Use back to determine who's pointing at us (IE assume new node
        // grafts into same place as old one)
        AstNode** nextnodepp = nullptr;
        if (this->m_backp->m_op1p == this) {
            nextnodepp = &(this->m_backp->m_op1p);
        } else if (this->m_backp->m_op2p == this) {
            nextnodepp = &(this->m_backp->m_op2p);
        } else if (this->m_backp->m_op3p == this) {
            nextnodepp = &(this->m_backp->m_op3p);
        } else if (this->m_backp->m_op4p == this) {
            nextnodepp = &(this->m_backp->m_op4p);
        } else if (this->m_backp->m_nextp == this) {
            nextnodepp = &(this->m_backp->m_nextp);
        }
        UASSERT_OBJ(nextnodepp, this, "Node's back doesn't point to forward to node itself");
        {
            VL_DO_DANGLING(nodep->accept(v), nodep);  // nodep to null as may be replaced
        }
        nodep = *nextnodepp;  // Grab new node from point where old was connected
    }
    return nodep;
}

#endif  // Guard
