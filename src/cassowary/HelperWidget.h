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
#include <vector>

namespace setu::cassowary {

class HelperWidget : public ConstraintWidget {
public:
    std::vector<ConstraintWidget*> mWidgets;

    HelperWidget() = default;
    virtual ~HelperWidget() = default;

    virtual void add(ConstraintWidget* widget);
    virtual void removeAllIds();
    
    void copy(ConstraintWidget* src, std::unordered_map<ConstraintWidget*, ConstraintWidget*>& map);
};

} // namespace setu::cassowary
