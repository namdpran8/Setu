/*
 * Copyright (C) 2016 The Android Open Source Project
 * Copyright (c) 2026 Pranshu Namdeo
 *
 * Rewritten and ported from ArrayLinkedVariables.java from the
 * Android Open Source Project (AOSP).
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * SPDX-License-Identifier: Apache-2.0
 */

// Original: ArrayLinkedVariables.java:1-702
//
// Array-based linked list for storing variable coefficients in a row.
// This is the hot inner data structure — traversed on every pivot.
//
// Copyright (C) 2016 The Android Open Source Project — Apache 2.0

#pragma once

#include "ArrayRow.h"
#include "SolverVariable.h"

#include <string>
#include <vector>

namespace setu::cassowary {

class Cache;

/// Ported from ArrayLinkedVariables.java:40
class ArrayLinkedVariables : public ArrayRowVariables {
public:
    static constexpr int NONE = -1;

    int mCurrentSize = 0;

    /// ArrayLinkedVariables.java:110 — constructor
    ArrayLinkedVariables(ArrayRow* arrayRow, Cache* cache);

    ~ArrayLinkedVariables() override = default;

    // ─── ArrayRowVariables interface ───

    void put(SolverVariable* variable, float value) override;
    void add(SolverVariable* v, float value, bool removeFromDefinition) override;
    float use(ArrayRow* definition, bool removeFromDefinition) override;
    float remove(SolverVariable* variable, bool removeFromDefinition) override;
    void clear() override;
    bool contains(SolverVariable* variable) override;
    int indexOf(SolverVariable* variable) override;
    void invert() override;
    void divideByAmount(float amount) override;
    int getCurrentSize() override;
    SolverVariable* getVariable(int index) override;
    float getVariableValue(int index) override;
    float get(SolverVariable* v) override;
    int sizeInBytes() override;
    void display() override;

    // ─── Additional public methods ───

    int getHead() const { return mHead; }
    int getId(int index) const { return mArrayIndices[index]; }
    float getValue(int index) const { return mArrayValues[index]; }
    int getNextIndice(int index) const { return mArrayNextIndices[index]; }
    bool hasAtLeastOnePositiveVariable();
    SolverVariable* getPivotCandidate();
    std::string toString() const;

private:
    static constexpr bool DEBUG = false;
    static constexpr float sEpsilon = 0.001f;

    ArrayRow* mRow;         // non-owning, our owner
    Cache* mCache;          // non-owning, system-wide cache

    int mRowSize = 8;       // current array capacity
    SolverVariable* mCandidate = nullptr;

    // Parallel arrays implementing the linked list.
    // reserve(8) at construction per user's perf guidance — avoids
    // first few reallocations during the hot pivot path.
    std::vector<int>   mArrayIndices;
    std::vector<int>   mArrayNextIndices;
    std::vector<float> mArrayValues;

    int mHead = NONE;
    int mLast = NONE;
    bool mDidFillOnce = false;
};

} // namespace setu::cassowary
