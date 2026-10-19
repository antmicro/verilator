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

#ifndef VERILATOR_V3CLASSCONTEXT_H_
#define VERILATOR_V3CLASSCONTEXT_H_

#include "config_build.h"
#include "verilatedos.h"

class AstNetlist;

//============================================================================

class V3ClassContext final {
public:
    // Before V3Width
    static void bindMembers(AstNetlist* nodep) VL_MT_DISABLED;
    // After V3Width
    static void bindCtorCalls(AstNetlist* nodep) VL_MT_DISABLED;
};

#endif  // Guard
