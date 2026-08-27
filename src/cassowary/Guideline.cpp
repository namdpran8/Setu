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

#include "Guideline.h"
#include "LinearSystem.h"
#include "ConstraintWidgetContainer.h"
#include "SolverVariable.h"

namespace setu::cassowary {

Guideline::Guideline() {
    mAnchor = &mLeft;
    mAnchors.clear();
    mAnchors.push_back(mAnchor);
    int count = mListAnchors.size();
    for (int i = 0; i < count; i++) {
        mListAnchors[i] = mAnchor;
    }
}

bool Guideline::allowedInBarrier() const {
    return true;
}

int Guideline::getRelativeBehaviour() const {
    if (mRelativePercent != -1.0f) {
        return RELATIVE_PERCENT;
    }
    if (mRelativeBegin != -1) {
        return RELATIVE_BEGIN;
    }
    if (mRelativeEnd != -1) {
        return RELATIVE_END;
    }
    return RELATIVE_UNKNOWN;
}

void Guideline::setOrientation(int orientation) {
    if (mOrientation == orientation) {
        return;
    }
    mOrientation = orientation;
    mAnchors.clear();
    if (mOrientation == VERTICAL) {
        mAnchor = &mLeft;
    } else {
        mAnchor = &mTop;
    }
    mAnchors.push_back(mAnchor);
    int count = mListAnchors.size();
    for (int i = 0; i < count; i++) {
        mListAnchors[i] = mAnchor;
    }
}

int Guideline::getOrientation() const {
    return mOrientation;
}

void Guideline::setMinimumPosition(int minimum) {
    mMinimumPosition = minimum;
}

int Guideline::getMinimumPosition() const {
    return mMinimumPosition;
}

ConstraintAnchor* Guideline::getAnchor(ConstraintAnchor::Type anchorType) {
    switch (anchorType) {
        case ConstraintAnchor::Type::LEFT:
        case ConstraintAnchor::Type::RIGHT:
            if (mOrientation == VERTICAL) {
                return mAnchor;
            }
            break;
        case ConstraintAnchor::Type::TOP:
        case ConstraintAnchor::Type::BOTTOM:
        case ConstraintAnchor::Type::BASELINE:
            if (mOrientation == HORIZONTAL) {
                return mAnchor;
            }
            break;
        case ConstraintAnchor::Type::CENTER:
        case ConstraintAnchor::Type::CENTER_X:
        case ConstraintAnchor::Type::CENTER_Y:
        case ConstraintAnchor::Type::NONE:
            return nullptr;
    }
    return nullptr;
}

std::string Guideline::getType() const {
    return "Guideline";
}

void Guideline::setGuideBegin(int value) {
    if (value > -1) {
        mRelativePercent = -1.0f;
        mRelativeBegin = value;
        mRelativeEnd = -1;
    }
}

int Guideline::getRelativeBegin() const {
    return mRelativeBegin;
}

void Guideline::setGuideEnd(int value) {
    if (value > -1) {
        mRelativePercent = -1.0f;
        mRelativeBegin = -1;
        mRelativeEnd = value;
    }
}

int Guideline::getRelativeEnd() const {
    return mRelativeEnd;
}

void Guideline::setGuidePercent(float value) {
    if (value > -1.0f) {
        mRelativePercent = value;
        mRelativeBegin = -1;
        mRelativeEnd = -1;
    }
}

float Guideline::getRelativePercent() const {
    return mRelativePercent;
}

void Guideline::addToSolver(LinearSystem* system, bool optimize) {
    ConstraintWidgetContainer* parent = dynamic_cast<ConstraintWidgetContainer*>(getParent());
    if (parent == nullptr) {
        return;
    }
    ConstraintAnchor* begin = parent->getAnchor(ConstraintAnchor::Type::LEFT);
    ConstraintAnchor* end = parent->getAnchor(ConstraintAnchor::Type::RIGHT);
    bool parentWrapContent = mParent != nullptr && mParent->mListDimensionBehaviors[DIMENSION_HORIZONTAL] == DimensionBehaviour::WRAP_CONTENT;

    if (mOrientation == HORIZONTAL) {
        begin = parent->getAnchor(ConstraintAnchor::Type::TOP);
        end = parent->getAnchor(ConstraintAnchor::Type::BOTTOM);
        parentWrapContent = mParent != nullptr && mParent->mListDimensionBehaviors[DIMENSION_VERTICAL] == DimensionBehaviour::WRAP_CONTENT;
    }

    if (mResolved && mAnchor->hasFinalValue()) {
        SolverVariable* guide = system->createObjectVariable(mAnchor);
        system->addEquality(guide, mAnchor->getFinalValue());
        if (mRelativeBegin != -1) {
            if (parentWrapContent) {
                system->addGreaterThan(system->createObjectVariable(end), guide, 0, SolverVariable::STRENGTH_EQUALITY);
            }
        } else if (mRelativeEnd != -1) {
            if (parentWrapContent) {
                SolverVariable* parentEnd = system->createObjectVariable(end);
                system->addGreaterThan(guide, system->createObjectVariable(begin), 0, SolverVariable::STRENGTH_EQUALITY);
                system->addGreaterThan(parentEnd, guide, 0, SolverVariable::STRENGTH_EQUALITY);
            }
        }
        mResolved = false;
        return;
    }

    SolverVariable* guide = system->createObjectVariable(mAnchor);
    SolverVariable* parentBegin = system->createObjectVariable(begin);

    if (mRelativeBegin != -1) {
        system->addEquality(guide, parentBegin, mRelativeBegin, SolverVariable::STRENGTH_FIXED);
        if (parentWrapContent) {
            system->addGreaterThan(system->createObjectVariable(end), guide, 0, SolverVariable::STRENGTH_EQUALITY);
        }
    } else if (mRelativeEnd != -1) {
        SolverVariable* parentEnd = system->createObjectVariable(end);
        system->addEquality(guide, parentEnd, -mRelativeEnd, SolverVariable::STRENGTH_FIXED);
        if (parentWrapContent) {
            system->addGreaterThan(guide, parentBegin, 0, SolverVariable::STRENGTH_EQUALITY);
            system->addGreaterThan(parentEnd, guide, 0, SolverVariable::STRENGTH_EQUALITY);
        }
    } else if (mRelativePercent != -1.0f) {
        SolverVariable* parentEnd = system->createObjectVariable(end);
        system->addConstraint(LinearSystem::createRowDimensionPercent(system, guide, parentEnd, mRelativePercent));
    }
}

void Guideline::updateFromSolver(LinearSystem* system, bool optimize) {
    if (getParent() == nullptr) {
        return;
    }
    int value = system->getObjectVariableValue(mAnchor);
    if (mOrientation == VERTICAL) {
        setX(value);
        setY(0);
        setHeight(getParent()->getHeight());
        setWidth(0);
    } else {
        setX(0);
        setY(value);
        setWidth(getParent()->getWidth());
        setHeight(0);
    }
}

} // namespace setu::cassowary
