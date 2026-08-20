#pragma once

#include "ArrayRow.h"

namespace setu::cassowary {

class Cache;
class SolverVariable;

class GoalRow : public ArrayRow {
public:
    // androidx/constraintlayout/core/GoalRow.java:21
    explicit GoalRow(Cache* cache);

    // androidx/constraintlayout/core/GoalRow.java:25
    void addError(SolverVariable* error) override;
};

} // namespace setu::cassowary
