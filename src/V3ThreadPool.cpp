// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Thread pool for Verilator itself
//
// Code available from: https://verilator.org
//
//*************************************************************************
//
// Copyright 2005-2020 by Wilson Snyder.  This program is free software; you can
// redistribute it and/or modify it under the terms of either the GNU
// Lesser General Public License Version 3 or the Perl Artistic License
// Version 2.0.
//
// Verilator is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
//*************************************************************************

#include "config_build.h"

#include "V3ThreadPool.h"

#include "V3Ast.h"
#include "V3Error.h"

V3ThreadPool::V3ThreadPool()
    : m_shutdown(false) {}

V3ThreadPool::~V3ThreadPool() { resize(0); }

void V3ThreadPool::resize(unsigned n) {
    UASSERT(m_queue.empty(), "Resizing busy thread pool");
    // Shut down old threads
    m_shutdown = true;
    m_cv.notify_all();
    while (!m_workers.empty()) {
        m_workers.front().join();
        m_workers.pop_front();
    }
    // Start new threads
    m_shutdown = false;
    for (unsigned int i = 0; i < n; i++) {
        m_workers.emplace_back(&V3ThreadPool::worker, this, i);
    }
}

void V3ThreadPool::worker(int id) {
    while (true) {
        // Wait for a notification
        std::unique_lock<std::mutex> lock(m_mutex);
        m_cv.wait(lock, [&] { return !m_queue.empty() || m_shutdown; });

        // Terminate if requested
        if (m_shutdown) return;

        UASSERT(!m_queue.empty(), "Job should be available");

        // Get the job
        job_t job = m_queue.front();
        m_queue.pop_front();

        // Unlock the lock so other threads in the pool can proceed
        lock.unlock();

        // Execute the job
        job();
    }
}

template <>
void V3ThreadPool::push_job<void>(std::shared_ptr<std::promise<void>>& prom,
                                  std::function<void()>&& f) {
    if (executeSynchronously()) {
        f();
        prom->set_value();
    } else {
        const std::lock_guard<std::mutex> lg(m_mutex);
        m_queue.push_back([prom, f] {
            f();
            prom->set_value();
        });
    }
}

// The global thread pool
V3ThreadPool v3ThreadPool;
