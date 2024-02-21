// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Backports of features from future C++ versions
//
// Code available from: https://verilator.org
//
//*************************************************************************
//
// Copyright 2003-2024 by Wilson Snyder. This program is free software; you
// can redistribute it and/or modify it under the terms of either the GNU
// Lesser General Public License Version 3 or the Perl Artistic License
// Version 2.0.
// SPDX-License-Identifier: LGPL-3.0-only OR Artistic-2.0
//
//*************************************************************************

#ifndef VERILATOR_V3STDFUTURE_H_
#define VERILATOR_V3STDFUTURE_H_

#include <functional>

namespace vlstd {

// C++17 is_invocable_v
template <typename F, typename... Args>
inline constexpr bool is_invocable_v
    = std::is_constructible_v<std::function<void(Args...)>,
                              std::reference_wrapper<std::remove_reference_t<F>>>;

// C++17 is_invocable_r_v
template <typename R, typename F, typename... Args>
inline constexpr bool is_invocable_r_v
    = std::is_constructible_v<std::function<R(Args...)>,
                              std::reference_wrapper<std::remove_reference_t<F>>>;

};  // namespace vlstd

#endif  // Guard
