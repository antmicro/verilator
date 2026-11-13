// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of either the GNU Lesser General Public License Version 3
// or the Perl Artistic License Version 2.0.
// SPDX-FileCopyrightText: 2026 <TODO>
// SPDX-License-Identifier: LGPL-3.0-only OR Artistic-2.0
//
//*************************************************************************

#include "svdpi.h"
#include "vpi_user.h"

static int writeValue(const char* path, int value, int flag) {
    const vpiHandle handle = vpi_handle_by_name(const_cast<char*>(path), nullptr);
    if (!handle) return 0;
    s_vpi_value val{};
    val.format = vpiIntVal;
    val.value.integer = value;
    vpi_put_value(handle, &val, nullptr, flag);
    vpi_release_handle(handle);
    return 1;
}

extern "C" int deposit(const char* path, int value) { return writeValue(path, value, vpiNoDelay); }

extern "C" int force_value(const char* path, int value) {
    return writeValue(path, value, vpiForceFlag);
}

extern "C" int release_value(const char* path) { return writeValue(path, 0, vpiReleaseFlag); }

extern "C" int read_value(const char* path) {
    const vpiHandle handle = vpi_handle_by_name(const_cast<char*>(path), nullptr);
    if (!handle) return -1;
    s_vpi_value val{};
    val.format = vpiIntVal;
    vpi_get_value(handle, &val);
    vpi_release_handle(handle);
    return val.value.integer;
}
