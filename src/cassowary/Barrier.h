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

#include "HelperWidget.h"
#include "LinearSystem.h"

namespace setu::cassowary {

class Barrier : public HelperWidget {
public:
    static constexpr int LEFT = 0;
    static constexpr int RIGHT = 1;
    static constexpr int TOP = 2;
    static constexpr int BOTTOM = 3;

private:
    int mBarrierType = LEFT;
    bool mAllowsMathComputation = true;
    int mMargin = 0;
    bool mResolved = false;

public:
    Barrier();
    Barrier(const std::string& debugName);
    virtual ~Barrier() = default;

    std::string getType() const override { return "Barrier"; }
    
    bool allowedInBarrier() const override;

    void setBarrierType(int barrierType);
    int getBarrierType() const;
    void setAllowsMathComputation(bool allow);
    bool allowsMathComputation() const;
    void setMargin(int margin);
    int getMargin() const;

    void markWidgets();
    void addToSolver(LinearSystem* system, bool optimize) override;
    
protected:
    bool allSolveVariablesHaveValues() const;
};

} // namespace setu::cassowary
