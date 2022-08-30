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

#ifndef _V3THREADPOOL_H_
#define _V3THREADPOOL_H_ 1

#include "V3Ast.h"

#include <condition_variable>
#include <functional>
#include <future>
#include <list>
#include <mutex>
#include <thread>

//============================================================================

class V3ThreadPool final {
public:
    V3ThreadPool();

    ~V3ThreadPool();

    // Resize thread pool to n workers (queue must be empty)
    void resize(unsigned n);

    // Enqueue a job for asynchronous execution
    template <typename T>
    std::future<T> enqueue(std::function<T()>&& f);

    // Apply function to each AstNode, following 'nextp()'
    template <typename T,
              typename std::enable_if<std::is_base_of<AstNode, T>::value, int>::type = 0>
    void foreach (T* nodep, std::function<void(T*)> && f);
    template <typename T,
              typename std::enable_if<std::is_base_of<AstNode, T>::value, int>::type = 0>
    void foreach (std::vector<T*> vec, std::function<void(T*)> && f);

private:
    typedef std::function<void()> job_t;

    std::list<job_t> m_queue;  // The job queue
    std::condition_variable m_cv;  // Used to wake up workers
    std::mutex m_mutex;  // Mutex for use by cv and and queue
    std::list<std::thread> m_workers;  // The worker threads

    bool m_shutdown;  // Flag signalling termination

    bool executeSynchronously() const { return m_workers.empty(); }

    template <typename T>
    void push_job(std::shared_ptr<std::promise<T>>& prom, std::function<T()>&& f);

    void worker(int id);
};

template <typename T>
std::future<T> V3ThreadPool::enqueue(std::function<T()>&& f) {
    std::shared_ptr<std::promise<T>> prom = std::make_shared<std::promise<T>>();
    std::future<T> result = prom->get_future();
    push_job(prom, std::move(f));
    m_cv.notify_one();
    return result;
}

template <typename T, typename std::enable_if<std::is_base_of<AstNode, T>::value, int>::type>
void V3ThreadPool::foreach (T* nodep, std::function<void(T*)> && f) {
    std::list<std::future<void>> results;
    // Enqueue calls
    for (; nodep; nodep = AstNode::privateCast<T, decltype(nodep->nextp())>(nodep->nextp())) {
        results.push_back(enqueue<void>([nodep, f] { f(nodep); }));
    }
    // Wait for results
    while (!results.empty()) {
        results.front().get();
        results.pop_front();
    }
}

template <typename T, typename std::enable_if<std::is_base_of<AstNode, T>::value, int>::type>
void V3ThreadPool::foreach (std::vector<T*> vec, std::function<void(T*)> && f) {
    std::list<std::future<void>> results;
    // Enqueue calls
    for (int i = 0; i < vec.size(); ++i) {
        results.push_back(enqueue<void>([vec, f, i] { f(vec[i]); }));
    }
    // Wait for results
    while (!results.empty()) {
        results.front().get();
        results.pop_front();
    }
}

template <typename T>
void V3ThreadPool::push_job(std::shared_ptr<std::promise<T>>& prom, std::function<T()>&& f) {
    if (executeSynchronously()) {
        prom->set_value(f());
    } else {
        const std::lock_guard<std::mutex> lg(m_mutex);
        m_queue.push_back([prom, f] { prom->set_value(f()); });
    }
}

template <>
void V3ThreadPool::push_job<void>(std::shared_ptr<std::promise<void>>& prom,
                                  std::function<void()>&& f);

// The global thread pool
extern V3ThreadPool v3ThreadPool;

#endif  // Guard
