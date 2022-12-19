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

#ifndef _V3THREADPOOL_H_
#define _V3THREADPOOL_H_ 1

#include "verilated_threads.h"

#include "V3Ast.h"

#include <condition_variable>
#include <functional>
#include <future>
#include <list>
#include <mutex>
#include <queue>
#include <thread>

//============================================================================

class V3ThreadPool;
// The global thread pool
extern V3ThreadPool v3ThreadPool;

class V3ThreadPool final {
    // MEMBERS
    using job_t = std::function<void()>;

    mutable VerilatedMutex m_mutex;  // Mutex for use by m_queue and m_cv
    std::queue<job_t> m_queue VL_GUARDED_BY(m_mutex);  // Queue of jobs
    std::condition_variable_any m_cv VL_GUARDED_BY(m_mutex);  // Conditions to wake up workers
    std::list<std::thread> m_workers;  // Worker threads

    bool m_shutdown = false;  // Flag signalling termination

public:
    // CONSTRUCTORS
    V3ThreadPool() = default;
    ~V3ThreadPool() { resize(0); }

    // METHODS
    // Resize thread pool to n workers (queue must be empty)
    void resize(unsigned n) VL_MT_SAFE;

    // Enqueue a job for asynchronous execution
    template <typename T>
    std::future<T> enqueue(std::function<T()>&& f) VL_MT_SAFE;

private:
    bool executeSynchronously() const VL_MT_SAFE { return m_workers.empty(); }

    template <typename T>
    void push_job(std::shared_ptr<std::promise<T>>& prom, std::function<T()>&& f) VL_MT_SAFE;

    void worker(int id) VL_MT_SAFE;

    static void startWorker(V3ThreadPool* selfp, int id) VL_MT_SAFE;
};

template <typename T>
std::future<T> V3ThreadPool::enqueue(std::function<T()>&& f) {
    std::shared_ptr<std::promise<T>> prom = std::make_shared<std::promise<T>>();
    std::future<T> result = prom->get_future();
    push_job(prom, std::move(f));
    const VerilatedLockGuard lock{m_mutex};
    m_cv.notify_one();
    return result;
}

template <typename T>
void V3ThreadPool::push_job(std::shared_ptr<std::promise<T>>& prom, std::function<T()>&& f) {
    if (executeSynchronously()) {
        prom->set_value(f());
    } else {
        const VerilatedLockGuard guard{m_mutex};
        m_queue.push([prom, f] { prom->set_value(f()); });
    }
}

template <>
void V3ThreadPool::push_job<void>(std::shared_ptr<std::promise<void>>& prom,
                                  std::function<void()>&& f);

#endif  // Guard
