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

#include "HelperWidget.h"
#include <algorithm>

namespace setu::cassowary {

void HelperWidget::add(ConstraintWidget* widget) {
    if (widget != this && std::find(mWidgets.begin(), mWidgets.end(), widget) == mWidgets.end()) {
        mWidgets.push_back(widget);
    }
}

void HelperWidget::removeAllIds() {
    mWidgets.clear();
}

void HelperWidget::copy(ConstraintWidget* src, std::unordered_map<ConstraintWidget*, ConstraintWidget*>& map) {
    ConstraintWidget::copy(src, map);
    if (auto srcHelper = dynamic_cast<HelperWidget*>(src)) {
        mWidgets.clear();
        for (ConstraintWidget* w : srcHelper->mWidgets) {
            auto it = map.find(w);
            if (it != map.end()) {
                mWidgets.push_back(it->second);
            }
        }
    }
}

} // namespace setu::cassowary
