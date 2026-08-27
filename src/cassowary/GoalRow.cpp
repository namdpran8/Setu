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
