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

#include "Barrier.h"
#include "ConstraintWidgetContainer.h"
#include "SolverVariable.h"
#include <iostream>

namespace setu::cassowary {

Barrier::Barrier() {
}

Barrier::Barrier(const std::string& debugName) : HelperWidget() {
    setDebugName(debugName);
}

bool Barrier::allowedInBarrier() const {
    return true;
}

void Barrier::setBarrierType(int barrierType) {
    mBarrierType = barrierType;
}

int Barrier::getBarrierType() const {
    return mBarrierType;
}

void Barrier::setAllowsMathComputation(bool allow) {
    mAllowsMathComputation = allow;
}

bool Barrier::allowsMathComputation() const {
    return mAllowsMathComputation;
}

void Barrier::setMargin(int margin) {
    mMargin = margin;
}

int Barrier::getMargin() const {
    return mMargin;
}

void Barrier::markWidgets() {
    for (ConstraintWidget* widget : mWidgets) {
        if (mBarrierType == LEFT || mBarrierType == RIGHT) {
            widget->setInBarrier(HORIZONTAL, true);
        } else if (mBarrierType == TOP || mBarrierType == BOTTOM) {
            widget->setInBarrier(VERTICAL, true);
        }
    }
}

bool Barrier::allSolveVariablesHaveValues() const {
    // Optimization check: if all referenced variables have final values,
    // the barrier itself is resolved directly without solver.
    // Simplifying this for now to return false to rely on the solver.
    return false;
}

void Barrier::addToSolver(LinearSystem* system, bool optimize) {
    mListAnchors[ANCHOR_LEFT]->resetFinalResolution();
    mListAnchors[ANCHOR_RIGHT]->resetFinalResolution();
    mListAnchors[ANCHOR_TOP]->resetFinalResolution();
    mListAnchors[ANCHOR_BOTTOM]->resetFinalResolution();
    
    if (mWidgets.empty()) {
        return;
    }

    ConstraintAnchor* position = nullptr;
    if (mBarrierType == LEFT || mBarrierType == RIGHT) {
        position = (mBarrierType == LEFT) ? &mLeft : &mRight;
    } else {
        position = (mBarrierType == TOP) ? &mTop : &mBottom;
    }

    bool hasMatchConstraintWidgets = false;
    for (ConstraintWidget* widget : mWidgets) {
        if (!mAllowsMathComputation && !widget->allowedInBarrier()) {
            continue;
        }
        if (mBarrierType == LEFT || mBarrierType == RIGHT) {
            if (widget->getHorizontalDimensionBehaviour() == DimensionBehaviour::MATCH_CONSTRAINT) {
                hasMatchConstraintWidgets = true;
            }
        } else {
            if (widget->getVerticalDimensionBehaviour() == DimensionBehaviour::MATCH_CONSTRAINT) {
                hasMatchConstraintWidgets = true;
            }
        }
    }

    bool isLower = (mBarrierType == LEFT || mBarrierType == TOP);

    SolverVariable* barrier = system->createObjectVariable(position);

    for (ConstraintWidget* widget : mWidgets) {
        if (!mAllowsMathComputation && !widget->allowedInBarrier()) {
            continue;
        }
        ConstraintAnchor* targetAnchor = nullptr;
        if (mBarrierType == LEFT) {
            targetAnchor = &widget->mLeft;
        } else if (mBarrierType == RIGHT) {
            targetAnchor = &widget->mRight;
        } else if (mBarrierType == TOP) {
            targetAnchor = &widget->mTop;
        } else if (mBarrierType == BOTTOM) {
            targetAnchor = &widget->mBottom;
        }

        SolverVariable* target = system->createObjectVariable(targetAnchor);
        if (isLower) {
            system->addLowerBarrier(barrier, target, mMargin, hasMatchConstraintWidgets);
        } else {
            system->addGreaterBarrier(barrier, target, mMargin, hasMatchConstraintWidgets);
        }
    }
}

} // namespace setu::cassowary
