/*
 * Copyright (c) 2026 Pranshu Namdeo
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "ArrayRow.h"
#include "Cache.h"
#include "SolverVariable.h"
#include <vector>
#include <string>

namespace setu::cassowary {

class SolverVariableValues : public ArrayRowVariables {
private:
    static constexpr bool DEBUG = false;
    static constexpr bool HASH = true;
    static constexpr float sEpsilon = 0.001f;
    const int mNone = -1;
    int mSize = 16;
    int mHashSize = 16;

    std::vector<int> mKeys;
    std::vector<int> mNextKeys;
    std::vector<int> mVariables;
    std::vector<float> mValues;
    std::vector<int> mPrevious;
    std::vector<int> mNext;
    int mCount = 0;
    int mHead = -1;

    ArrayRow* mRow;
    Cache* mCache;

    // SolverVariableValues.java:132
    void increaseSize();
    
    // SolverVariableValues.java:169
    void addToHashMap(SolverVariable* variable, int index);
    
    // SolverVariableValues.java:185
    void removeFromHashMap(SolverVariable* variable);
    
    // SolverVariableValues.java:214
    void addVariable(int index, SolverVariable* variable, float value);
    
    // SolverVariableValues.java:224
    int findEmptySlot();
    
    // SolverVariableValues.java:233
    void insertVariable(int index, SolverVariable* variable, float value);

public:
    // SolverVariableValues.java:25
    SolverVariableValues(ArrayRow* row, Cache* cache);
    virtual ~SolverVariableValues() = default;

    // SolverVariableValues.java:32
    int getCurrentSize() override;

    // SolverVariableValues.java:37
    SolverVariable* getVariable(int index) override;

    // SolverVariableValues.java:55
    float getVariableValue(int index) override;

    // SolverVariableValues.java:70
    bool contains(SolverVariable* variable) override;

    // SolverVariableValues.java:75
    int indexOf(SolverVariable* variable) override;

    // SolverVariableValues.java:101
    float get(SolverVariable* variable) override;

    // SolverVariableValues.java:109
    void clear() override;

    // SolverVariableValues.java:311
    int sizeInBytes() override;

    // SolverVariableValues.java:256
    void put(SolverVariable* variable, float value) override;

    // SolverVariableValues.java:316
    float remove(SolverVariable* v, bool removeFromDefinition) override;

    // SolverVariableValues.java:353
    void add(SolverVariable* v, float value, bool removeFromDefinition) override;

    // SolverVariableValues.java:374
    float use(ArrayRow* definition, bool removeFromDefinition) override;

    // SolverVariableValues.java:449
    void invert() override;

    // SolverVariableValues.java:464
    void divideByAmount(float amount) override;

    // SolverVariableValues.java:482
    std::string toString();

    // SolverVariableValues.java:493
    void display();
};

} // namespace setu::cassowary
