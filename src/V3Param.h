// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Param modules/signals together
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

#ifndef VERILATOR_V3PARAM_H_
#define VERILATOR_V3PARAM_H_

#include "config_build.h"
#include "verilatedos.h"

class AstNetlist;

//============================================================================

class V3Param final {
public:
    // Name of the AstLambdaArgRef for the index of an instance array element in a pin of a
    // bind target (see V3LinkCells)
    static constexpr const char* BIND_ELEM_NAME = "__VbindElem";
    // Prefix of the localparam for a bind to instances; the name ends with the first and last
    // number of its "__Vbind<number>" parameters, the value is the name of the target
    static constexpr const char* BIND_TARGET_PREFIX = "__VbindTarget";
    static void param(AstNetlist* rootp) VL_MT_DISABLED;
    static void finalizeDeferredParams(AstNetlist* rootp) VL_MT_DISABLED;
};

#endif  // Guard
