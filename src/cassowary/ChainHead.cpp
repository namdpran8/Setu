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

#include "ChainHead.h"
#include "ConstraintWidget.h"
#include <iostream>

namespace setu::cassowary {

ChainHead::ChainHead(ConstraintWidget* first, int orientation, bool isRtl) 
    : mFirst(first), mOrientation(orientation), mIsRtl(isRtl) {
}

void ChainHead::define() {
    if (mDefined) return;
    mDefined = true;
    mOptimizes = true;
    
    ConstraintWidget* widget = mFirst;
    ConstraintWidget* next = mFirst;
    bool done = false;
    
    int offset = mOrientation * 2;
    
    while (!done) {
        mWidgetsCount++;
        widget->mNextChainWidget[mOrientation] = nullptr;
        widget->mListNextMatchConstraintsWidget[mOrientation] = nullptr;
        
        if (widget->getVisibility() != ConstraintWidget::GONE) {
            mVisibleWidgets++;
            if (mFirstVisibleWidget == nullptr) {
                mFirstVisibleWidget = widget;
            }
            if (mLastVisibleWidget != nullptr) {
                mLastVisibleWidget->mNextChainWidget[mOrientation] = widget;
            }
            mLastVisibleWidget = widget;
            
            if (widget->mListDimensionBehaviors[mOrientation] == ConstraintWidget::DimensionBehaviour::MATCH_CONSTRAINT) {
                if (widget->mResolvedMatchConstraintDefault[mOrientation] == ConstraintWidget::MATCH_CONSTRAINT_SPREAD ||
                    widget->mResolvedMatchConstraintDefault[mOrientation] == ConstraintWidget::MATCH_CONSTRAINT_RATIO ||
                    widget->mResolvedMatchConstraintDefault[mOrientation] == ConstraintWidget::MATCH_CONSTRAINT_PERCENT) {
                    
                    mWidgetsMatchCount++;
                    float weight = widget->mWeight[mOrientation];
                    if (weight > 0) {
                        mTotalWeight += widget->mWeight[mOrientation];
                    }
                    
                    if (isMatchConstraintIntersectionDependsOnResolvedWidgets(widget, mOrientation)) {
                        if (weight < 0) {
                            mHasUndefinedWeights = true;
                        } else {
                            mHasComplexMatchWeights = true;
                        }
                        if (mWeightedMatchConstraintsWidgets.empty()) {
                            mWeightedMatchConstraintsWidgets.push_back(widget);
                        } else {
                            mWeightedMatchConstraintsWidgets.push_back(widget);
                        }
                    } else {
                        if (weight < 0) {
                            mHasUndefinedWeights = true;
                        } else {
                            mHasComplexMatchWeights = true;
                        }
                        if (mWeightedMatchConstraintsWidgets.empty()) {
                            mWeightedMatchConstraintsWidgets.push_back(widget);
                        } else {
                            mWeightedMatchConstraintsWidgets.push_back(widget);
                        }
                    }
                    
                    if (mFirstMatchConstraintWidget == nullptr) {
                        mFirstMatchConstraintWidget = widget;
                    }
                    if (mLastMatchConstraintWidget != nullptr) {
                        mLastMatchConstraintWidget->mListNextMatchConstraintsWidget[mOrientation] = widget;
                    }
                    mLastMatchConstraintWidget = widget;
                }
            }
        }
        
        if (widget != mFirst) {
            mTotalMargins += widget->mListAnchors[offset]->getMargin();
        }
        mTotalMargins += widget->mListAnchors[offset + 1]->getMargin();
        mTotalSize += widget->mListAnchors[offset]->getMargin();
        mTotalSize += widget->mListAnchors[offset + 1]->getMargin();
        
        if (widget->getVisibility() != ConstraintWidget::GONE) {
            if (mOrientation == ConstraintWidget::HORIZONTAL) {
                mTotalSize += widget->getWidth();
            } else {
                mTotalSize += widget->getHeight();
            }
        }
        
        // Find next widget in the chain
        ConstraintAnchor* nextAnchor = widget->mListAnchors[offset + 1]->mTarget;
        if (nextAnchor != nullptr) {
            ConstraintWidget* nextWidget = nextAnchor->mOwner;
            if (nextWidget->mListAnchors[offset]->mTarget != nullptr &&
                nextWidget->mListAnchors[offset]->mTarget->mOwner == widget) {
                next = nextWidget;
            } else {
                next = nullptr;
            }
        } else {
            next = nullptr;
        }
        
        if (next != nullptr) {
            widget = next;
        } else {
            done = true;
        }
    }
    
    mLast = widget;
    
    if (mOrientation == ConstraintWidget::HORIZONTAL && mIsRtl) {
        mHead = mLast;
    } else {
        mHead = mFirst;
    }
    
    mHasRatio = mHasComplexMatchWeights && mHasUndefinedWeights;
}

bool ChainHead::isMatchConstraintIntersectionDependsOnResolvedWidgets(ConstraintWidget* widget, int orientation) {
    if (widget->mListDimensionBehaviors[orientation] == ConstraintWidget::DimensionBehaviour::MATCH_CONSTRAINT) {
        if (widget->mResolvedMatchConstraintDefault[orientation] == ConstraintWidget::MATCH_CONSTRAINT_RATIO) {
            return true;
        }
    }
    return false;
}

} // namespace setu::cassowary
