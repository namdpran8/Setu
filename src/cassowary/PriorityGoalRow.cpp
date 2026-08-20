// PriorityGoalRow.cpp — Ported from androidx.constraintlayout.core.PriorityGoalRow (PriorityGoalRow.java)
// Original: PriorityGoalRow.java:1-268
//
// Copyright (C) 2020 The Android Open Source Project — Apache 2.0

#include "PriorityGoalRow.h"

#include <cmath>
#include <algorithm>
#include <string>

namespace setu::cassowary {

/// PriorityGoalRow.java:43
void PriorityGoalRow::GoalVariableAccessor::init(SolverVariable* variable) {
    mVariable = variable;
}

/// PriorityGoalRow.java:47
bool PriorityGoalRow::GoalVariableAccessor::addToGoal(SolverVariable* other, float value) {
    if (mVariable->inGoal) {
        bool empty = true;
        for (int i = 0; i < SolverVariable::MAX_STRENGTH; i++) {
            mVariable->mGoalStrengthVector[i] += other->mGoalStrengthVector[i] * value;
            float v = mVariable->mGoalStrengthVector[i];
            if (std::abs(v) < EPSILON) {
                mVariable->mGoalStrengthVector[i] = 0.0f;
            } else {
                empty = false;
            }
        }
        if (empty) {
            mRow->removeGoal(mVariable);
        }
    } else {
        for (int i = 0; i < SolverVariable::MAX_STRENGTH; i++) {
            float strength = other->mGoalStrengthVector[i];
            if (strength != 0.0f) {
                float v = value * strength;
                if (std::abs(v) < EPSILON) {
                    v = 0.0f;
                }
                mVariable->mGoalStrengthVector[i] = v;
            } else {
                mVariable->mGoalStrengthVector[i] = 0.0f;
            }
        }
        return true;
    }
    return false;
}

/// PriorityGoalRow.java:80
void PriorityGoalRow::GoalVariableAccessor::add(SolverVariable* other) {
    for (int i = 0; i < SolverVariable::MAX_STRENGTH; i++) {
        mVariable->mGoalStrengthVector[i] += other->mGoalStrengthVector[i];
        float value = mVariable->mGoalStrengthVector[i];
        if (std::abs(value) < EPSILON) {
            mVariable->mGoalStrengthVector[i] = 0.0f;
        }
    }
}

/// PriorityGoalRow.java:90
bool PriorityGoalRow::GoalVariableAccessor::isNegative() const {
    for (int i = SolverVariable::MAX_STRENGTH - 1; i >= 0; i--) {
        float value = mVariable->mGoalStrengthVector[i];
        if (value > 0.0f) {
            return false;
        }
        if (value < 0.0f) {
            return true;
        }
    }
    return false;
}

/// PriorityGoalRow.java:103
bool PriorityGoalRow::GoalVariableAccessor::isSmallerThan(SolverVariable* other) const {
    for (int i = SolverVariable::MAX_STRENGTH - 1; i >= 0; i--) {
        float comparedValue = other->mGoalStrengthVector[i];
        float value = mVariable->mGoalStrengthVector[i];
        if (value == comparedValue) {
            continue;
        }
        return value < comparedValue;
    }
    return false;
}

/// PriorityGoalRow.java:115
bool PriorityGoalRow::GoalVariableAccessor::isNull() const {
    for (int i = 0; i < SolverVariable::MAX_STRENGTH; i++) {
        if (mVariable->mGoalStrengthVector[i] != 0.0f) {
            return false;
        }
    }
    return true;
}

/// PriorityGoalRow.java:124
void PriorityGoalRow::GoalVariableAccessor::reset() {
    std::fill(mVariable->mGoalStrengthVector, mVariable->mGoalStrengthVector + SolverVariable::MAX_STRENGTH, 0.0f);
}

/// PriorityGoalRow.java:128
std::string PriorityGoalRow::GoalVariableAccessor::toString() const {
    std::string result = "[ ";
    if (mVariable != nullptr) {
        for (int i = 0; i < SolverVariable::MAX_STRENGTH; i++) {
            result += std::to_string(mVariable->mGoalStrengthVector[i]) + " ";
        }
    }
    result += "] ";
    if (mVariable != nullptr) {
        result += mVariable->toString();
    } else {
        result += "null";
    }
    return result;
}

/// PriorityGoalRow.java:150
PriorityGoalRow::PriorityGoalRow(Cache* cache)
    : ArrayRow(cache), mCache(cache), mAccessor(this) {
    mArrayGoals.resize(mTableSize, nullptr);
    mSortArray.resize(mTableSize, nullptr);
}

/// PriorityGoalRow.java:142
void PriorityGoalRow::clear() {
    mNumGoals = 0;
    mConstantValue = 0.0f;
}

/// PriorityGoalRow.java:155
bool PriorityGoalRow::isEmpty() {
    return mNumGoals == 0;
}

/// PriorityGoalRow.java:162
SolverVariable* PriorityGoalRow::getPivotCandidate(LinearSystem* system, std::vector<bool>& avoid) {
    int pivot = NOT_FOUND;
    for (int i = 0; i < mNumGoals; i++) {
        SolverVariable* variable = mArrayGoals[i];
        if (avoid[variable->id]) {
            continue;
        }
        mAccessor.init(variable);
        if (pivot == NOT_FOUND) {
            if (mAccessor.isNegative()) {
                pivot = i;
            }
        } else if (mAccessor.isSmallerThan(mArrayGoals[pivot])) {
            pivot = i;
        }
    }
    if (pivot == NOT_FOUND) {
        return nullptr;
    }
    return mArrayGoals[pivot];
}

/// PriorityGoalRow.java:185
void PriorityGoalRow::addError(SolverVariable* error) {
    mAccessor.init(error);
    mAccessor.reset();
    error->mGoalStrengthVector[error->strength] = 1.0f;
    addToGoal(error);
}

/// PriorityGoalRow.java:193
void PriorityGoalRow::addToGoal(SolverVariable* variable) {
    if (mNumGoals + 1 > (int)mArrayGoals.size()) {
        mArrayGoals.resize(mArrayGoals.size() * 2, nullptr);
        mSortArray.resize(mArrayGoals.size() * 2, nullptr);
    }
    mArrayGoals[mNumGoals] = variable;
    mNumGoals++;

    if (mNumGoals > 1 && mArrayGoals[mNumGoals - 1]->id > variable->id) {
        for (int i = 0; i < mNumGoals; i++) {
            mSortArray[i] = mArrayGoals[i];
        }
        std::sort(mSortArray.begin(), mSortArray.begin() + mNumGoals, 
                  [](SolverVariable* variable1, SolverVariable* variable2) {
                      return variable1->id < variable2->id;
                  });
        for (int i = 0; i < mNumGoals; i++) {
            mArrayGoals[i] = mSortArray[i];
        }
    }

    variable->inGoal = true;
    variable->addToRow(this);
}

/// PriorityGoalRow.java:220
void PriorityGoalRow::removeGoal(SolverVariable* variable) {
    for (int i = 0; i < mNumGoals; i++) {
        if (mArrayGoals[i] == variable) {
            for (int j = i; j < mNumGoals - 1; j++) {
                mArrayGoals[j] = mArrayGoals[j + 1];
            }
            mNumGoals--;
            variable->inGoal = false;
            return;
        }
    }
}

/// PriorityGoalRow.java:233
void PriorityGoalRow::updateFromRow(LinearSystem* system, ArrayRow* definition, bool removeFromDefinition) {
    SolverVariable* goalVariable = definition->mVariable;
    if (goalVariable == nullptr) {
        return;
    }

    ArrayRowVariables* rowVariables = definition->variables.get();
    int currentSize = rowVariables->getCurrentSize();
    for (int i = 0; i < currentSize; i++) {
        SolverVariable* solverVariable = rowVariables->getVariable(i);
        float value = rowVariables->getVariableValue(i);
        mAccessor.init(solverVariable);
        if (mAccessor.addToGoal(goalVariable, value)) {
            addToGoal(solverVariable);
        }
        mConstantValue += definition->mConstantValue * value;
    }
    removeGoal(goalVariable);
}

/// PriorityGoalRow.java:256
std::string PriorityGoalRow::toString() const {
    std::string result = "";
    result += " goal -> (" + std::to_string(mConstantValue) + ") : ";
    for (int i = 0; i < mNumGoals; i++) {
        SolverVariable* v = mArrayGoals[i];
        GoalVariableAccessor accessor(const_cast<PriorityGoalRow*>(this));
        accessor.init(v);
        result += accessor.toString() + " ";
    }
    return result;
}

} // namespace setu::cassowary
