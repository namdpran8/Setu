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
#include <memory>

namespace setu::cassowary {

class Cache;

class WidgetContainer : public ConstraintWidget {
public:
    std::vector<std::unique_ptr<ConstraintWidget>> mChildren;

    WidgetContainer();
    WidgetContainer(int x, int y, int width, int height);
    WidgetContainer(int width, int height);
    virtual ~WidgetContainer() = default;

    virtual void reset() override;
    
    void add(std::unique_ptr<ConstraintWidget> widget);
    std::unique_ptr<ConstraintWidget> remove(ConstraintWidget* widget);
    
    const std::vector<std::unique_ptr<ConstraintWidget>>& getChildren() const;
    
    class ConstraintWidgetContainer* getRootConstraintContainer();
    
    virtual void setOffset(int x, int y) override;
    
    virtual void layout();
    
    virtual void resetSolverVariables(Cache* cache) override;
    
    void removeAllChildren();
};

} // namespace setu::cassowary
