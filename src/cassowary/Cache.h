// Cache.h — Ported from androidx.constraintlayout.core.Cache (Cache.java)
// Original: Cache.java:1-27
//
// Cache for common solver objects (pools of ArrayRow and SolverVariable).
// OWNERSHIP: Cache does NOT own the objects in its pools or mIndexedVariables.
// LinearSystem owns all SolverVariable and ArrayRow instances; Cache merely
// provides recycling pools and an index for fast lookup by variable id.
//
// Copyright (C) 2016 The Android Open Source Project — Apache 2.0

#pragma once

#include "Pools.h"

#include <vector>

namespace setu::cassowary {

// Forward declarations — full definitions in their respective headers
class ArrayRow;
class SolverVariable;

/// Ported from Cache.java:21 — public class Cache
struct Cache {
    /// Cache.java:22 — Pools.Pool<ArrayRow> mOptimizedArrayRowPool
    SimplePool<ArrayRow> mOptimizedArrayRowPool{256};

    /// Cache.java:23 — Pools.Pool<ArrayRow> mArrayRowPool
    SimplePool<ArrayRow> mArrayRowPool{256};

    /// Cache.java:24 — Pools.Pool<SolverVariable> mSolverVariablePool
    SimplePool<SolverVariable> mSolverVariablePool{256};

    /// Cache.java:25 — SolverVariable[] mIndexedVariables = new SolverVariable[32]
    /// Index from variable.id -> SolverVariable*. Grows via LinearSystem::increaseTableSize().
    /// Non-owning pointers. Initialized to nullptr (vector default).
    std::vector<SolverVariable*> mIndexedVariables;

    Cache() : mIndexedVariables(32, nullptr) {}
};

} // namespace setu::cassowary
