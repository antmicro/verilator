// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
//
// Code available from: https://verilator.org
//
// Copyright 2003-2022 by Wilson Snyder. This program is free software; you can
// redistribute it and/or modify it under the terms of either the GNU
// Lesser General Public License Version 3 or the Perl Artistic License
// Version 2.0.
// SPDX-License-Identifier: LGPL-3.0-only OR Artistic-2.0
//
//*************************************************************************
#ifndef VERILATOR_VERILATED_DYNAMIC_SCHEDULER_H_
#define VERILATOR_VERILATED_DYNAMIC_SCHEDULER_H_

#ifndef VERILATOR_VERILATED_H_INTERNAL_
#error "verilated_dynamic_scheduler.h should only be included by verilated.h"
#endif

// Dynamic scheduler-related includes
#include <vector>
#include <map>
#include <functional>
#include <utility>
#include <queue>
#include <unordered_map>
#include <unordered_set>
#include <functional>

// Some preprocessor magic to support both Clang and GCC coroutines with both libc++ and stdlibc++
#ifdef __clang__
#ifdef _LIBCPP_VERSION
// Using libc++, coroutine library is in std::experimental
#include <experimental/coroutine>
namespace std {
// Bring std::experimental into the std namespace
using namespace experimental;
}  // namespace std
#else
// Using stdlibc++, coroutine library is in std namespace
#define __cpp_impl_coroutine 1  // clang doesn't define this, but it's needed in <coroutine>
#include <coroutine>
namespace std {
// Bring coroutine library into std::experimental, as clang expects it to be there
namespace experimental = ::std;
}  // namespace std
#endif
#else  // Not clang
#include <coroutine>
#endif

//=============================================================================
// VerilatedDelayedQueue
// Priority queue where the priority is the simulation time at which a coroutine should resume, and
// the value is the coroutine to be resumed.
class VerilatedDelayedQueue final {
private:
    // TYPES
    using DelayedCoro = std::pair<double, std::coroutine_handle<>>;
    struct Cmp {
        bool operator()(const DelayedCoro& lhs, const DelayedCoro& rhs) {
            return lhs.first > rhs.first;
        }
    };
    using DelayedPriorityQueue = std::priority_queue<DelayedCoro, std::vector<DelayedCoro>, Cmp>;

    // MEMBERS
    DelayedPriorityQueue m_queue;  // Actual encapsulated queue
    VerilatedMutex m_mutex;  // Protects m_queue

public:
    // METHODS
    // Push a suspended coroutine to be resumed at the given simulation time
    void push(double time, std::coroutine_handle<> coro) {
        const VerilatedLockGuard guard{m_mutex};
        m_queue.push(std::make_pair(time, coro));
    }

    // Resume coroutines waiting for the given simulation time
    void resume(double time) {
        VerilatedLockGuard guard{m_mutex};
        while (!m_queue.empty() && m_queue.top().first <= time) {
            auto coro = m_queue.top().second;
            m_queue.pop();
            guard.unlock();
            coro();
            guard.lock();
        }
    }

    // Simulation time of the next time slot
    double nextTimeSlot() {
        const VerilatedLockGuard guard{m_mutex};
        if (!m_queue.empty())
            return m_queue.top().first;
        else
            return VL_TIME_D();
    }

    // Is the queue empty?
    bool empty() {
        const VerilatedLockGuard guard{m_mutex};
        return m_queue.empty();
    }

    // Used by coroutines for co_awaiting a certain simulation time
    auto operator[](double time) {
        const VerilatedLockGuard guard{m_mutex};
        struct Awaitable {
            VerilatedDelayedQueue& queue;  // The queue to push the coroutine to
            double time;  // Simulation time at which the coroutine should be resumed

            bool await_ready() { return false; }  // Always suspend

            void await_suspend(std::coroutine_handle<> coro) { queue.push(time, coro); }

            void await_resume() {}
        };
        return Awaitable{*this, time};
    }
};

// Event variable-related type aliases
using VerilatedEvent = CData;
using VerilatedEventSet = std::unordered_set<VerilatedEvent*>;
using VerilatedCoroutineArray = std::vector<std::coroutine_handle<>>;

//=============================================================================
// VerilatedEventToCoroMap
// Mapping from events to suspended coroutines. Used by VerilatedEventDispatcher.
class VerilatedEventToCoroMap final {
private:
    // TYPES
    struct Hash {
        size_t operator()(const VerilatedEventSet& set) const {
            size_t result = 0;
            for (auto event : set) result ^= std::hash<VerilatedEvent*>()(event);
            return result;
        }
    };
    using EventSetToCoroMap
        = std::unordered_multimap<VerilatedEventSet, std::coroutine_handle<>, Hash>;
    using EventToEventSetMap = std::unordered_multimap<VerilatedEvent*, VerilatedEventSet>;

    // MEMBERS
    EventToEventSetMap m_eventsToEventSets;  // Events mapped to event sets...
    EventSetToCoroMap m_eventSetsToCoros;  // ...which are mapped to coroutines

    // METHODS
    // Move coroutines waiting on the specified events to the array
    void move(const VerilatedEventSet& events, VerilatedCoroutineArray& coros) {
        auto range = m_eventSetsToCoros.equal_range(events);
        for (auto it = range.first; it != range.second; ++it) coros.push_back(it->second);
        m_eventSetsToCoros.erase(range.first, range.second);
    }

public:
    // Move coroutines waiting on the specified event to the array
    void move(VerilatedEvent* event, VerilatedCoroutineArray& coros) {
        auto range = m_eventsToEventSets.equal_range(event);
        for (auto it = range.first; it != range.second; ++it) move(it->second, coros);
    }

    // Are there coroutines waiting on this set of events?
    bool contains(const VerilatedEventSet& events) {
        auto range = m_eventSetsToCoros.equal_range(events);
        return range.first != range.second;
    }

    // Insert a coroutine waiting on the specified events
    void insert(const VerilatedEventSet& events, std::coroutine_handle<> coro) {
        for (auto event : events) m_eventsToEventSets.insert(std::make_pair(event, events));
        m_eventSetsToCoros.insert(std::make_pair(events, coro));
    }

    // Erase all coroutines waiting on the given event
    void erase(VerilatedEvent* event) {
        auto range = m_eventsToEventSets.equal_range(event);
        for (auto it = range.first; it != range.second; ++it) m_eventSetsToCoros.erase(it->second);
    }
};

//=============================================================================
// VerilatedEventDispatcher
// Object that manages event variables and suspended coroutines waiting on those event vars.
// When an event gets triggered, the corresponding coroutine isn't resumed immediately. This is to
// allow for all processes to get to their timing controls, so that order of execution of processes
// doesn't matter as much. Coroutines are resumed later by the 'resumeTriggered` function. The only
// exception to this is when an event gets triggered twice – then all coroutines up to the first
// trigger get resumed. This is neccessary so we don't lose track of events getting triggered and
// the order they're triggered in.
class VerilatedEventDispatcher final {
private:
    // MEMBERS
    VerilatedEventToCoroMap m_eventsToCoros;  // Mapping of events to coroutines
    std::vector<VerilatedEvent*> m_triggeredEvents;  // Events triggered in the current time slot
    VerilatedEventSet m_eventsToReset;  // Events to be reset at the start of a time slot
    VerilatedCoroutineArray
        m_readyCoros;  // Coroutines ready for resumption in the current time slot
    VerilatedMutex m_mutex;  // Protects m_eventSetsToCoros, m_triggeredEvents, m_eventsToCoros, and m_readyCoros

    // METHODS
    // Move coroutines waiting on events from m_triggeredEvents to m_readyCoros
    void readyTriggered() {
        VerilatedLockGuard guard{m_mutex};
        std::vector<VerilatedEvent*> queue = std::move(m_triggeredEvents);
        for (auto event : queue) m_eventsToCoros.move(event, m_readyCoros);
    }

    // Are there no ready coroutines?
    bool readyEmpty() {
        const VerilatedLockGuard guard{m_mutex};
        return m_readyCoros.empty();
    }

    // Are there coroutines waiting on this set of events? (wrapper for thread safety)
    bool isSetWaitedOn(const VerilatedEventSet& events) {
        const VerilatedLockGuard guard{m_mutex};
        return m_eventsToCoros.contains(events);
    }

public:
    // Insert the specified coroutine waiting on the given set of events
    void insert(const VerilatedEventSet& events, std::coroutine_handle<> coro) {
        if (isSetWaitedOn(events)) { readyTriggered(); }
        VerilatedLockGuard guard{m_mutex};
        m_eventsToCoros.insert(events, coro);
    }

    // Resume coroutines waiting on all triggered events (and keep doing it until no events get
    // triggered); also trigger delayed assignment event
    void resumeTriggered(VerilatedEvent& dlyEvent) {
        do {
            resumeTriggered();
            trigger(dlyEvent);
            readyTriggered();
        } while (!readyEmpty());
    }

    // Resume coroutines waiting on all triggered events (and keep doing it until no events get
    // triggered)
    void resumeTriggered() {
        readyTriggered();
        while (!readyEmpty()) {
            m_mutex.lock();
            VerilatedCoroutineArray queue = std::move(m_readyCoros);
            m_mutex.unlock();
            for (auto coro : queue) coro();
            readyTriggered();
        }
    }

    // Reset all events triggered in the previous time slot
    void resetTriggered() {
        const VerilatedLockGuard guard{m_mutex};
        for (auto event : m_eventsToReset) *event = 0;
        m_eventsToReset.clear();
    }

    // Stop waiting on the given event
    void cancel(VerilatedEvent& event) {
        const VerilatedLockGuard guard{m_mutex};
        m_eventsToCoros.erase(&event);
        m_eventsToReset.erase(&event);
    }
    template <size_t depth> void cancel(VlUnpacked<VerilatedEvent, depth>& events) {
        for (size_t i = 0; i < depth; i++) cancel(events[i]);
    }

    // Move the given event to the triggered event queue
    void trigger(VerilatedEvent& event) {
        event = 1;
        const VerilatedLockGuard guard{m_mutex};
        m_eventsToReset.insert(&event);
        m_triggeredEvents.push_back(&event);
    }

    // Used by coroutines for co_awaiting a certain set of events
    auto operator[](VerilatedEventSet&& events) {
        struct Awaitable {
            VerilatedEventDispatcher& dispatcher;  // The dispatcher to put the events in
            VerilatedEventSet events;  // Events the coroutine will wait on

            bool await_ready() { return false; }  // Always suspend

            void await_suspend(std::coroutine_handle<> coro) { dispatcher.insert(events, coro); }

            void await_resume() {}
        };
        return Awaitable{*this, std::move(events)};
    }
};

//=============================================================================
// VerilatedCoroutine
// Return value of a coroutine. Used for chaining coroutine suspension/resumption.
class VerilatedCoroutine final {
private:
    // TYPES
    struct VerilatedPromise {
        VerilatedCoroutine get_return_object() { return {this}; }

        // Never suspend at the start of the coroutine
        std::suspend_never initial_suspend() { return {}; }

        // Never suspend at the end of the coroutine (thanks to this, the coroutine will clean up
        // after itself)
        std::suspend_never final_suspend() noexcept {
            // Indicate to the return object that the coroutine has finished
            if (m_coro) m_coro->m_promise = nullptr;
            // If there is a continuation, resume it
            if (m_continuation) m_continuation();
            return {};
        }

        void unhandled_exception() { std::abort(); }
        void return_void() const {}

        std::coroutine_handle<>
            m_continuation;  // Coroutine that should be resumed after this one finishes
        VerilatedCoroutine* m_coro = nullptr;  // Pointer to the coroutine return object
    };

    // MEMBERS
    VerilatedPromise* m_promise;  // The promise created for this coroutine

public:
    // TYPES
    using promise_type = VerilatedPromise;  // promise_type has to be public

    // CONSTRUCTORS
    // Construct
    VerilatedCoroutine(VerilatedPromise* p)
        : m_promise(p) {
        m_promise->m_coro = this;
    }

    // Move. Update the pointers each time the return object is moved
    VerilatedCoroutine(VerilatedCoroutine&& other)
        : m_promise(std::exchange(other.m_promise, nullptr)) {
        if (m_promise) m_promise->m_coro = this;
    }

    // Indicate to the promise that the return object is gone
    ~VerilatedCoroutine() {
        if (m_promise) m_promise->m_coro = nullptr;
    }

    // METHODS
    // Suspend the awaiter if the coroutine is suspended (the promise exists)
    bool await_ready() const noexcept { return !m_promise; }

    // Set the awaiting coroutine as the continuation of the current coroutine
    void await_suspend(std::coroutine_handle<> coro) { m_promise->m_continuation = coro; }

    void await_resume() const noexcept {}
};

#endif  // Guard
