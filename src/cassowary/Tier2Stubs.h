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

#include "ConstraintWidget.h"
#include <set>

#include "Guideline.h"

#include "Barrier.h"

#include "ChainHead.h"

namespace setu::cassowary {

class VirtualLayout : public ConstraintWidget {
public:
    virtual ~VirtualLayout() = default;
    bool contains(const std::set<ConstraintWidget*>& widgets) { return false; }
};

} // namespace setu::cassowary
