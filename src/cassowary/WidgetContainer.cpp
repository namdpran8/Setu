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

#include "WidgetContainer.h"
#include "ConstraintWidgetContainer.h"

namespace setu::cassowary {

// 36: WidgetContainer()
WidgetContainer::WidgetContainer() : ConstraintWidget() {}

// 47: WidgetContainer(int x, int y, int width, int height)
WidgetContainer::WidgetContainer(int x, int y, int width, int height) 
    : ConstraintWidget(x, y, width, height) {}

// 57: WidgetContainer(int width, int height)
WidgetContainer::WidgetContainer(int width, int height) 
    : ConstraintWidget(width, height) {}

// 61: reset()
void WidgetContainer::reset() {
    mChildren.clear();
    ConstraintWidget::reset();
}

// 72: add(ConstraintWidget widget)
void WidgetContainer::add(std::unique_ptr<ConstraintWidget> widget) {
    if (!widget) return;
    
    ConstraintWidget* rawWidget = widget.get();
    if (rawWidget->getParent() != nullptr) {
        WidgetContainer* container = static_cast<WidgetContainer*>(rawWidget->getParent());
        widget = container->remove(rawWidget); // Extracts it from old parent
    }
    rawWidget->setParent(this);
    mChildren.push_back(std::move(widget));
}

// 98: remove(ConstraintWidget widget)
std::unique_ptr<ConstraintWidget> WidgetContainer::remove(ConstraintWidget* widget) {
    for (auto it = mChildren.begin(); it != mChildren.end(); ++it) {
        if (it->get() == widget) {
            std::unique_ptr<ConstraintWidget> extracted = std::move(*it);
            mChildren.erase(it);
            extracted->reset();
            return extracted;
        }
    }
    return nullptr;
}

// 108: getChildren()
const std::vector<std::unique_ptr<ConstraintWidget>>& WidgetContainer::getChildren() const {
    return mChildren;
}

// 117: getRootConstraintContainer()
ConstraintWidgetContainer* WidgetContainer::getRootConstraintContainer() {
    ConstraintWidget* item = this;
    ConstraintWidget* parent = item->getParent();
    ConstraintWidgetContainer* container = nullptr;
    
    if (auto cwc = dynamic_cast<ConstraintWidgetContainer*>(this)) {
        container = cwc;
    }
    
    while (parent != nullptr) {
        item = parent;
        parent = item->getParent();
        if (auto cwc = dynamic_cast<ConstraintWidgetContainer*>(item)) {
            container = cwc;
        }
    }
    return container;
}

// 145: setOffset(int x, int y)
void WidgetContainer::setOffset(int x, int y) {
    ConstraintWidget::setOffset(x, y);
    for (auto& widget : mChildren) {
        widget->setOffset(getRootX(), getRootY());
    }
}

// 158: layout()
void WidgetContainer::layout() {
    for (auto& widget : mChildren) {
        if (auto container = dynamic_cast<WidgetContainer*>(widget.get())) {
            container->layout();
        }
    }
}

// 171: resetSolverVariables(Cache cache)
void WidgetContainer::resetSolverVariables(Cache* cache) {
    ConstraintWidget::resetSolverVariables(cache);
    for (auto& widget : mChildren) {
        widget->resetSolverVariables(cache);
    }
}

// 182: removeAllChildren()
void WidgetContainer::removeAllChildren() {
    mChildren.clear();
}

} // namespace setu::cassowary
