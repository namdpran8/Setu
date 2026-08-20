// Pools.h — Ported from androidx.constraintlayout.core.Pools (Pools.java)
// Original: https://github.com/nicklasbekkgaard/constraintlayout/blob/main/constraintlayout/core/src/main/java/androidx/constraintlayout/core/Pools.java
//
// Simple object pool template. In Java, this avoids GC churn; in C++ it avoids
// repeated allocation/deallocation overhead on the solver hot path.
// The pool stores raw non-owning pointers — the caller (LinearSystem) owns lifetimes.
//
// Copyright (C) 2015 The Android Open Source Project — Apache 2.0

#pragma once

#include <cstddef>
#include <vector>
#include <cassert>

namespace setu::cassowary {

/// Ported from Pools.java:83 — SimplePool<T>
/// Fixed-capacity LIFO pool of reusable objects.
/// Stores raw non-owning pointers; caller is responsible for object lifetimes.
template <typename T>
class SimplePool {
public:
    /// Pools.java:94 — SimplePool(int maxPoolSize)
    explicit SimplePool(std::size_t maxPoolSize)
        : mMaxPoolSize(maxPoolSize) {
        assert(maxPoolSize > 0);
        mPool.reserve(maxPoolSize);
    }

    /// Pools.java:103 — T acquire()
    /// Returns a pooled instance, or nullptr if the pool is empty.
    T* acquire() {
        if (mPool.empty()) {
            return nullptr;
        }
        T* instance = mPool.back();
        mPool.pop_back();
        return instance;
    }

    /// Pools.java:115 — boolean release(T instance)
    /// Returns the instance to the pool. Returns true if accepted, false if pool is full.
    bool release(T* instance) {
        if (mPool.size() >= mMaxPoolSize) {
            return false;
        }
        mPool.push_back(instance);
        return true;
    }

    /// Pools.java:130 — void releaseAll(T[] variables, int count)
    /// Batch-release up to `count` instances back to the pool.
    void releaseAll(T** variables, int count) {
        if (variables == nullptr) return;
        for (int i = 0; i < count; i++) {
            if (mPool.size() >= mMaxPoolSize) {
                break;
            }
            mPool.push_back(variables[i]);
        }
    }

    /// Overload accepting std::vector — used by LinearSystem::reset()
    void releaseAll(std::vector<T*>& variables, int count) {
        for (int i = 0; i < count && i < static_cast<int>(variables.size()); i++) {
            if (mPool.size() >= mMaxPoolSize) {
                break;
            }
            mPool.push_back(variables[i]);
        }
    }

private:
    std::size_t mMaxPoolSize;
    std::vector<T*> mPool;
};

} // namespace setu::cassowary
