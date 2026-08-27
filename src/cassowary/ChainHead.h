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

#include <vector>

namespace setu::cassowary {

class ConstraintWidget;
class Chain;

class ChainHead {
    friend class Chain;
protected:
    ConstraintWidget* mFirst = nullptr;
    ConstraintWidget* mFirstVisibleWidget = nullptr;
    ConstraintWidget* mLast = nullptr;
    ConstraintWidget* mLastVisibleWidget = nullptr;
    ConstraintWidget* mHead = nullptr;
    ConstraintWidget* mFirstMatchConstraintWidget = nullptr;
    ConstraintWidget* mLastMatchConstraintWidget = nullptr;

    std::vector<ConstraintWidget*> mWeightedMatchConstraintsWidgets;

    int mWidgetsCount = 0;
    int mWidgetsMatchCount = 0;
    float mTotalWeight = 0.0f;
    int mVisibleWidgets = 0;
    int mTotalSize = 0;
    int mTotalMargins = 0;
    bool mOptimizes = false;

private:
    int mOrientation;
    bool mIsRtl;
    bool mHasUndefinedWeights = false;
    bool mHasComplexMatchWeights = false;
    bool mHasRatio = false;
    bool mDefined = false;

public:
    ChainHead(ConstraintWidget* first, int orientation, bool isRtl);
    virtual ~ChainHead() = default;

    void define();

    ConstraintWidget* getFirst() const { return mFirst; }
    ConstraintWidget* getFirstVisibleWidget() const { return mFirstVisibleWidget; }
    ConstraintWidget* getLast() const { return mLast; }
    ConstraintWidget* getLastVisibleWidget() const { return mLastVisibleWidget; }
    ConstraintWidget* getHead() const { return mHead; }
    ConstraintWidget* getFirstMatchConstraintWidget() const { return mFirstMatchConstraintWidget; }
    ConstraintWidget* getLastMatchConstraintWidget() const { return mLastMatchConstraintWidget; }
    float getTotalWeight() const { return mTotalWeight; }
    void setTotalWeight(float totalWeight) { mTotalWeight = totalWeight; }

private:
    static bool isMatchConstraintIntersectionDependsOnResolvedWidgets(ConstraintWidget* widget, int orientation);
};

} // namespace setu::cassowary
