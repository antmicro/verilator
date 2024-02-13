// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: V3ThreadPool benchmark
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
//
// Building and running:
//
//     mkdir -p src/obj_opt
//     make -C src/obj_opt -f ../Makefile_obj V3ThreadPool_benchmark
//     ./src/obj_opt/V3ThreadPool_benchmark -timeout 5 -threads 2
//

// clang-format off
#include "config_build.h"
#ifndef HAVE_CONFIG_PACKAGE
# error "Something failed during ./configure as config_package.h is incomplete. Perhaps you used autoreconf, don't."
#endif
// clang-format on

#include "verilatedos.h"

// Cheat for speed and compile .cpp files into one object TODO: Reconsider
#ifndef V3ERROR_NO_GLOBAL_
#define V3ERROR_NO_GLOBAL_
#endif

#ifndef V3OPTION_PARSER_NO_VOPTION_BOOL
#define V3OPTION_PARSER_NO_VOPTION_BOOL
#endif

#include "V3Error.h"
static int debug() { return V3Error::debugDefault(); }
#include "V3OptionParser.h"
#include "V3ThreadPool.h"

#include <atomic>
#include <chrono>
#include <cstdio>
#include <memory>
#include <vector>

// clang-format off
#include "V3Error.cpp"
#include "V3String.cpp"
#include "V3OptionParser.cpp"
#include "V3Os.cpp"
#include "V3Mutex.h"
// clang-format on

struct Options {
    int m_threads = 2;
    int m_timeout = 5;

    void parseOptsList(int argc, char** argv);
};

void Options::parseOptsList(int argc, char** argv) VL_MT_DISABLED {
    V3OptionParser parser;
    V3OptionParser::AppendHelper DECL_OPTION{parser};
    V3OPTION_PARSER_DECL_TAGS;

    DECL_OPTION("-threads", Set, &m_threads);
    DECL_OPTION("-timeout", Set, &m_timeout);
    parser.finalize();
    for (int i = 0; i < argc;) {
        if (argv[i][0] == '-') {
            if (const int consumed = parser.parse(i, argc, argv)) {
                i += consumed;
                continue;
            }
        }
        v3fatal("Invalid option: " << argv[i] << parser.getSuggestion(argv[i]));
        ++i;  // LCOV_EXCL_LINE
    }
}

int main(int argc, char** argv) {
    // General initialization
    std::ios::sync_with_stdio();

    Options opt;
    {
        const V3MtDisabledLockGuard mtDisabler{v3MtDisabledLock()};
        opt.parseOptsList(argc - 1, argv + 1);

        UASSERT(opt.m_threads >= 0, "Invalid threads count");
        UASSERT(opt.m_timeout >= 0, "Invalid timeout");

        V3MutexConfig::s().configure(true);
    }

    V3ThreadPool::s().resize(opt.m_threads + 1);
    std::unique_ptr<std::atomic_uint64_t[]> counters
        = std::make_unique<std::atomic_uint64_t[]>(opt.m_threads + 1);

    uint64_t scheduled_jobs = 0;

    bool stop = false;
    V3SharedMutex stopLock;

    std::deque<std::future<void>> futures;

    {
        const auto expiration
            = std::chrono::high_resolution_clock::now() + std::chrono::seconds(opt.m_timeout);

        while (std::chrono::high_resolution_clock::now() < expiration) {
            for (int i = 0; i < 100; ++i) {
                auto f = V3ThreadPool::s().enqueue([counters = counters.get(), &stop, &stopLock] {
                    static thread_local uint64_t counter = 0;
                    V3SharedLockGuard l{stopLock};
                    if (!stop) {
                        ++counter;
                        counters[V3ThreadPool::currentThreadId()].store(counter,
                                                                        std::memory_order_release);
                    }
                });
                futures.push_back(std::move(f));
            }
            scheduled_jobs += 100;
        }

        V3ExclusiveLockGuard l{stopLock};
        stop = true;
    }
    uint64_t executed_jobs = 0;
    for (int i = 0; i < opt.m_threads + 1; ++i) {
        executed_jobs += counters[i].load(std::memory_order_consume);
    }

    std::printf("Elapsed time [s]:        %10d\n", opt.m_timeout);
    std::printf("Worker threads:          %10d\n", opt.m_threads);
    std::printf("Scheduled jobs:          %10lu\n", scheduled_jobs);
    std::printf("Executed jobs:           %10lu (%.6f%%)\n", executed_jobs,
                1.0 * executed_jobs / scheduled_jobs * 100.0);
    std::printf("Avg exec. jobs/thread:   %10.0f\n",
                1.0 * executed_jobs / std::max(1, opt.m_threads));
    std::printf("Avg time/job [µs]:       %10.3f\n", opt.m_timeout * 1000000.0 / executed_jobs);
    std::printf("Avg thread*time/job [µs]:%10.3f\n",
                opt.m_timeout * std::max(1, opt.m_threads) * 1000000.0 / executed_jobs);

    return 0;
}
