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
