// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Thread pool for Verilator itself
//
// Code available from: https://verilator.org
//
//*************************************************************************
//
// Copyright 2005-2022 by Wilson Snyder.  This program is free software; you can
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

// The global thread pool
V3ThreadPool v3ThreadPool;

void V3ThreadPool::resize(unsigned n) {
    VerilatedLockGuard lock{m_mutex};
    UASSERT(m_queue.empty(), "Resizing busy thread pool");
    // Shut down old threads
    m_shutdown = true;
    m_cv.notify_all();
    lock.unlock();
    while (!m_workers.empty()) {
        m_workers.front().join();
        m_workers.pop_front();
    }
    lock.lock();
    // Start new threads
    m_shutdown = false;
    for (unsigned int i = 1; i < n; i++) {
        m_workers.emplace_back(&V3ThreadPool::startWorker, this, i);
    }
}

void V3ThreadPool::startWorker(V3ThreadPool* selfp, int id) { selfp->worker(id); }

void V3ThreadPool::worker(int id) {
    while (true) {
        // Wait for a notification
        job_t job;
        {
            VerilatedLockGuard lock(m_mutex);
            m_cv.wait(lock, [&]() VL_REQUIRES(m_mutex) { return !m_queue.empty() || m_shutdown; });

            // Terminate if requested
            if (m_shutdown) return;

            // Get the job
            UASSERT(!m_queue.empty(), "Job should be available");

            job = m_queue.front();
            m_queue.pop();
        }

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
        const VerilatedLockGuard lock{m_mutex};
        m_queue.push([prom, f] {
            f();
            prom->set_value();
        });
    }
}
