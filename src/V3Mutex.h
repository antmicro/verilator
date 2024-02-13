// -*- mode: C++; c-file-style: "cc-mode" -*-
//*************************************************************************
// DESCRIPTION: Verilator: Thread pool for Verilator itself
//
// Code available from: https://verilator.org
//
//*************************************************************************
//
// Copyright 2005-2023 by Wilson Snyder.  This program is free software; you can
// redistribute it and/or modify it under the terms of either the GNU
// Lesser General Public License Version 3 or the Perl Artistic License
// Version 2.0.
// SPDX-License-Identifier: LGPL-3.0-only OR Artistic-2.0
//
//*************************************************************************
///
/// \file
/// \brief Verilator Mutex and LockGuard used only in verilator.
///
/// This file defines Mutex and LockGuard that is used in verilator
/// source code. It shouldn't be used by verilated code.
/// In contrast to VerilatedMutex that is used by verilated code,
/// this mutex can be configured and disabled when verilation is using
/// single thread.
///
/// This implementation also allows using different base mutex class in
/// wrapper V3Mutex class.
///
//*************************************************************************

#ifndef VERILATOR_V3MUTEX_H_
#define VERILATOR_V3MUTEX_H_ 1

#include "verilatedos.h"

#include <cassert>
#include <condition_variable>
#include <cstdlib>
#include <functional>
#include <mutex>
#include <shared_mutex>

#define VL_LOCK_SPINS 50000  /// Number of times to spin for a mutex before yielding

// MutexConfig class that allows to configure how mutex and lockgurads behave
// once configured and locked, it cannot be changed. Configuration and lock needs to be
// done before starting any additional threads.
class V3MutexConfig final {
private:
    // Allows to disable mutexes and lockguards.
    // Use carefully as it can cause undefined behavior when used inappropriately.
    // All mutexes needs to be unlocked.
    bool m_enable = false;  // Allows locking on mutexes, default don't lock mutexes
    bool m_lockConfig = false;  // After set, configuration cannot be changed

    V3MutexConfig() = default;
    ~V3MutexConfig() = default;

public:
    static V3MutexConfig& s() VL_MT_SAFE {
        static V3MutexConfig s;
        return s;
    }

    // configures class
    void configure(bool enable) VL_MT_UNSAFE {
        if (!s().m_lockConfig) {
            s().m_enable = enable;
            s().m_lockConfig = true;
        } else {
            // requires <iostream>
            // avoided to reduce compile time
            // std::cerr << "%Error: V3Mutex already configured." << std::endl;
            std::abort();
        }
    }
    bool lockConfig() VL_MT_SAFE { return m_lockConfig; }
    bool enable() VL_MT_SAFE { return m_enable; }
};

/// Mutex, wrapped to allow -fthread_safety checks
template <typename T>
class VL_CAPABILITY("mutex") V3MutexImp final {
private:
    T m_mutex;  // Mutex

public:
    /// Construct mutex (without locking it)
    V3MutexImp() = default;
    ~V3MutexImp() = default;
    VL_UNCOPYABLE(V3MutexImp);
    // For -Wthread-safety-negative
    const V3MutexImp& operator!() const { return *this; }
    /// Acquire/lock mutex
    void lock() VL_ACQUIRE() VL_MT_SAFE {
        if (V3MutexConfig::s().enable()) { m_mutex.lock(); }
    }
    /// Release/unlock mutex
    void unlock() VL_RELEASE() VL_MT_SAFE {
        if (V3MutexConfig::s().enable()) { m_mutex.unlock(); }
    }
    /// Try to acquire mutex.  Returns true on success, and false on failure.
    bool try_lock() VL_TRY_ACQUIRE(true) VL_MT_SAFE {
        return V3MutexConfig::s().enable() ? m_mutex.try_lock() : true;
    }
    /// Assume that the mutex is already held. Purely for Clang thread safety analyzer.
    void assumeLocked() VL_ASSERT_CAPABILITY(this) VL_MT_SAFE {}
    void assumeUnlocked() VL_ASSERT_CAPABILITY(!this) VL_MT_SAFE {}
    /// Pretend that the mutex is being unlocked. Purely for Clang thread safety analyzer.
    void pretendUnlock() VL_RELEASE() VL_MT_SAFE {}
    /// Acquire/lock mutex and check for stop request
    /// It tries to lock the mutex and if it fails, it check if stop request was send.
    /// It returns after locking mutex.
    /// This function should be extracted to V3ThreadPool, but due to clang thread-safety
    /// limitations it needs to be placed here.
    void lockCheckStopRequest(std::function<void()> checkStopRequestFunction)
        VL_ACQUIRE() VL_MT_SAFE {
        if (V3MutexConfig::s().enable()) {
            while (true) {
                checkStopRequestFunction();
                if (m_mutex.try_lock()) return;
                VL_CPU_RELAX();
            }
        }
    }
};

using V3Mutex = V3MutexImp<std::mutex>;
using V3RecursiveMutex = V3MutexImp<std::recursive_mutex>;

#if defined(VL_SHARED_MUTEX_IMPLEMENTATION_PTHREAD) && false

// TODO(mglb): Wrap std::shared_timed_mutex and ignore timing methods
// https://en.cppreference.com/w/cpp/thread/shared_timed_mutex

// PThreads-based implementation of shared mutex
class VL_CAPABILITY("shared mutex") V3SharedMutex final {
    pthread_rwlock_t m_mutex;

public:
    V3SharedMutex() {
        if (V3MutexConfig::s().enable()) { pthread_rwlock_init(&m_mutex, nullptr); }
    }
    ~V3SharedMutex() {
        if (V3MutexConfig::s().enable()) { pthread_rwlock_destroy(&m_mutex); }
    }

    V3SharedMutex(const V3SharedMutex&) = delete;
    V3SharedMutex(V3SharedMutex&&) = delete;

    V3SharedMutex& operator=(const V3SharedMutex&) = delete;
    V3SharedMutex& operator=(V3SharedMutex&&) = delete;

    // For -Wthread-safety-negative
    const V3SharedMutex& operator!() const { return *this; }

    // Exclusive locking

    void lock() VL_ACQUIRE() VL_MT_SAFE {
        if (V3MutexConfig::s().enable()) { pthread_rwlock_wrlock(&m_mutex); }
    }

    // TODO(mglb): bool try_lock();

    void unlock() VL_RELEASE() VL_MT_SAFE { pthread_rwlock_unlock(&m_mutex); }

    // Shared locking

    void lock_shared() VL_ACQUIRE_SHARED() VL_MT_SAFE {
        if (V3MutexConfig::s().enable()) { pthread_rwlock_rdlock(&m_mutex); }
    }

    // TODO(mglb): bool try_lock_shared();

    void unlock_shared() VL_RELEASE_SHARED() VL_MT_SAFE {
        if (V3MutexConfig::s().enable()) { pthread_rwlock_unlock(&m_mutex); }
    }

    void assumeLocked() VL_ASSERT_CAPABILITY(this) VL_MT_SAFE {}

    void assumeUnlocked() VL_ASSERT_CAPABILITY(!this) VL_MT_SAFE {}
};

#else

// std::shared_timed_mutex wrapper without timeout support.
// std::shared_mutex is not used as it has been added in C++17.
class VL_CAPABILITY("shared mutex") V3SharedMutex final {
    std::shared_timed_mutex m_mutex;

public:
    V3SharedMutex() = default;
    ~V3SharedMutex() = default;

    V3SharedMutex(const V3SharedMutex&) = delete;
    V3SharedMutex(V3SharedMutex&&) = delete;

    V3SharedMutex& operator=(const V3SharedMutex&) = delete;
    V3SharedMutex& operator=(V3SharedMutex&&) = delete;

    // For -Wthread-safety-negative
    const V3SharedMutex& operator!() const { return *this; }

    // Exclusive locking

    void lock() VL_ACQUIRE() {
        if (V3MutexConfig::s().enable()) m_mutex.lock();
    }

    // TODO(mglb): bool try_lock();

    void unlock() VL_RELEASE() {
        if (V3MutexConfig::s().enable()) m_mutex.unlock();
    }

    // Shared locking

    void lock_shared() VL_ACQUIRE_SHARED() {
        if (V3MutexConfig::s().enable()) m_mutex.lock_shared();
    }

    // TODO(mglb): bool try_lock_shared();

    void unlock_shared() VL_RELEASE_SHARED() {
        if (V3MutexConfig::s().enable()) m_mutex.unlock_shared();
    }

    void assumeLocked() VL_ASSERT_CAPABILITY(this) VL_MT_SAFE {}

    void assumeUnlocked() VL_ASSERT_CAPABILITY(!this) VL_MT_SAFE {}
};

#endif

/// Lock guard for mutex (ala std::unique_lock), wrapped to allow -fthread_safety checks
template <typename T>
class VL_SCOPED_CAPABILITY V3LockGuardImp final {
    VL_UNCOPYABLE(V3LockGuardImp);

private:
    T& m_mutexr;

public:
    /// Lock given mutex and hold it for the object lifetime.
    explicit V3LockGuardImp(T& mutexr) VL_ACQUIRE(mutexr) VL_MT_SAFE
        : m_mutexr(mutexr) {  // Need () or GCC 4.8 false warning
        mutexr.lock();
    }
    /// Take already locked mutex, and and hold the lock for the object lifetime.
    explicit V3LockGuardImp(T& mutexr, std::adopt_lock_t) VL_REQUIRES(mutexr) VL_MT_SAFE
        : m_mutexr(mutexr) {  // Need () or GCC 4.8 false warning
    }

    /// Unlock the mutex
    ~V3LockGuardImp() VL_RELEASE() { m_mutexr.unlock(); }
};

using V3LockGuard = V3LockGuardImp<V3Mutex>;
using V3RecursiveLockGuard = V3LockGuardImp<V3RecursiveMutex>;
using V3ExclusiveLockGuard = V3LockGuardImp<V3SharedMutex>;

class VL_SCOPED_CAPABILITY V3SharedLockGuard final {
    VL_UNCOPYABLE(V3SharedLockGuard);

private:
    V3SharedMutex& m_mutexr;

public:
    /// Acquire shared lock on given mutex and hold it for the object lifetime.
    explicit V3SharedLockGuard(V3SharedMutex& mutexr) VL_ACQUIRE_SHARED(mutexr) VL_MT_SAFE
        : m_mutexr(mutexr) {  // Need () or GCC 4.8 false warning
        mutexr.lock_shared();
    }
    /// Take already locked mutex, and and hold the lock for the object lifetime.
    explicit V3SharedLockGuard(V3SharedMutex& mutexr, std::adopt_lock_t)
        VL_REQUIRES_SHARED(mutexr) VL_MT_SAFE
        : m_mutexr(mutexr) {  // Need () or GCC 4.8 false warning
    }

    /// Unlock the mutex
    ~V3SharedLockGuard() VL_RELEASE() { m_mutexr.unlock_shared(); }
};

// clang-format off

#define VL_LOCK_GUARD(sym, mutex) \
    V3LockGuardImp<std::remove_const<std::remove_reference<decltype(mutex)>::type>::type> \
            sym{mutex}
#define VL_LOCK_GUARD_ADOPT(sym, mutex) \
    V3LockGuardImp<std::remove_const<std::remove_reference<decltype(mutex)>::type>::type> \
            sym {mutex, std::adopt_lock_t {}}

// Even through there is only one shared mutex type currently, provide macro-wrappers for
// consistency.
#define VL_SHARED_LOCK_GUARD(sym, mutex) \
    V3SharedLockGuard sym{mutex}
#define VL_SHARED_LOCK_GUARD_ADOPT(sym, mutex) \
    V3SharedLockGuard sym{mutex, std::adopt_lock_t {}}

// clang-format on

#endif  // guard
