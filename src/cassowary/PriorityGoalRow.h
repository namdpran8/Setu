// PriorityGoalRow.h — Ported from androidx.constraintlayout.core.PriorityGoalRow (PriorityGoalRow.java)
// Original: PriorityGoalRow.java:1-268
//
// Implements a row containing goals taking in account priorities.
//
// Copyright (C) 2020 The Android Open Source Project — Apache 2.0

#pragma once

#include "ArrayRow.h"
#include "Cache.h"
#include "SolverVariable.h"
#include "LinearSystem.h"

#include <string>
#include <vector>

namespace setu::cassowary {

/// Ported from PriorityGoalRow.java:25 — public class PriorityGoalRow extends ArrayRow
class PriorityGoalRow : public ArrayRow {
public:
    /// PriorityGoalRow.java:26
    static constexpr float EPSILON = 0.0001f;
    /// PriorityGoalRow.java:27
    static constexpr bool DEBUG = false;
    /// PriorityGoalRow.java:160
    static constexpr int NOT_FOUND = -1;

private:
    /// PriorityGoalRow.java:29
    int mTableSize = 128;
    /// PriorityGoalRow.java:30
    std::vector<SolverVariable*> mArrayGoals;
    /// PriorityGoalRow.java:31
    std::vector<SolverVariable*> mSortArray;
    /// PriorityGoalRow.java:32
    int mNumGoals = 0;
    /// PriorityGoalRow.java:148
    Cache* mCache;

    /// PriorityGoalRow.java:35 — class GoalVariableAccessor
    struct GoalVariableAccessor {
        SolverVariable* mVariable = nullptr;
        PriorityGoalRow* mRow = nullptr;

        /// PriorityGoalRow.java:39
        explicit GoalVariableAccessor(PriorityGoalRow* row) : mRow(row) {}

        /// PriorityGoalRow.java:43
        void init(SolverVariable* variable);

        /// PriorityGoalRow.java:47
        bool addToGoal(SolverVariable* other, float value);

        /// PriorityGoalRow.java:80
        void add(SolverVariable* other);

        /// PriorityGoalRow.java:90
        bool isNegative() const;

        /// PriorityGoalRow.java:103
        bool isSmallerThan(SolverVariable* other) const;

        /// PriorityGoalRow.java:115
        bool isNull() const;

        /// PriorityGoalRow.java:124
        void reset();

        /// PriorityGoalRow.java:128
        std::string toString() const;
    };

    /// PriorityGoalRow.java:33
    GoalVariableAccessor mAccessor;

    /// PriorityGoalRow.java:193
    void addToGoal(SolverVariable* variable);

    /// PriorityGoalRow.java:220
    void removeGoal(SolverVariable* variable);

public:
    /// PriorityGoalRow.java:150
    explicit PriorityGoalRow(Cache* cache);

    /// PriorityGoalRow.java:142
    void clear() override;

    /// PriorityGoalRow.java:155
    bool isEmpty() override;

    /// PriorityGoalRow.java:162
    SolverVariable* getPivotCandidate(LinearSystem* system, std::vector<bool>& avoid) override;

    /// PriorityGoalRow.java:185
    void addError(SolverVariable* error) override;

    /// PriorityGoalRow.java:233
    void updateFromRow(LinearSystem* system, ArrayRow* definition, bool removeFromDefinition) override;

    /// PriorityGoalRow.java:256
    std::string toString() const;
};

} // namespace setu::cassowary
