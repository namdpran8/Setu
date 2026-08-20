#include "GoalRow.h"
#include "Cache.h"
#include "SolverVariable.h"

namespace setu::cassowary {

// androidx/constraintlayout/core/GoalRow.java:21
GoalRow::GoalRow(Cache* cache)
    : ArrayRow(cache) {
}

// androidx/constraintlayout/core/GoalRow.java:25
void GoalRow::addError(SolverVariable* error) {
    ArrayRow::addError(error);
    // error variables in the goal shouldn't be tracked (we only care if they are
    // in the system rows)
    error->usageInRowCount--;
}

} // namespace setu::cassowary
