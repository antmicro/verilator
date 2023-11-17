// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
//
// Code available from: https://verilator.org
//
// Copyright 2022 by Wilson Snyder. This program is free software; you can
// redistribute it and/or modify it under the terms of either the GNU Lesser
// General Public License Version 3 or the Perl Artistic License Version 2.0.
// SPDX-License-Identifier: LGPL-3.0-only OR Artistic-2.0
//
//*************************************************************************
#ifndef VERILATOR_VERILATED_RANDOM_H_
#define VERILATOR_VERILATED_RANDOM_H_

#include <crave/ConstrainedRandom.hpp>

template <typename Func>
inline IData VL_RANDOMIZE_WITH(const crave::Generator& constraint, Func with_func) {
    crave::Generator gen;
    gen.merge(constraint);
    gen.hard(with_func(QData{}, gen));
    return gen.next();
}

#endif  // Guard
