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

#include "ConstraintWidget.h"
#include "Tier2Stubs.h"
#include "ConstraintWidgetContainer.h"



#include "LinearSystem.h"
#include "Cache.h"
#include <iostream>
#include <algorithm>
#include <cmath>
#include <cassert>

namespace setu::cassowary {

// 106: setFinalFrame
void ConstraintWidget::setFinalFrame(int left, int top, int right, int bottom, int baseline, int orientation) {
    setFrame(left, top, right, bottom);
    setBaselineDistance(baseline);
    if (orientation == HORIZONTAL) {
        mResolvedHorizontal = true;
        mResolvedVertical = false;
    } else if (orientation == VERTICAL) {
        mResolvedHorizontal = false;
        mResolvedVertical = true;
    } else if (orientation == BOTH) {
        mResolvedHorizontal = true;
        mResolvedVertical = true;
    } else {
        mResolvedHorizontal = false;
        mResolvedVertical = false;
    }
}

// 130: setFinalLeft
void ConstraintWidget::setFinalLeft(int x1) {
    mLeft.setFinalValue(x1);
    mX = x1;
}

// 136: setFinalTop
void ConstraintWidget::setFinalTop(int y1) {
    mTop.setFinalValue(y1);
    mY = y1;
}

// 142: resetSolvingPassFlag
void ConstraintWidget::resetSolvingPassFlag() {
    mHorizontalSolvingPass = false;
    mVerticalSolvingPass = false;
}

// 147: isHorizontalSolvingPassDone
bool ConstraintWidget::isHorizontalSolvingPassDone() const {
    return mHorizontalSolvingPass;
}

// 151: isVerticalSolvingPassDone
bool ConstraintWidget::isVerticalSolvingPassDone() const {
    return mVerticalSolvingPass;
}

// 156: markHorizontalSolvingPassDone
void ConstraintWidget::markHorizontalSolvingPassDone() {
    mHorizontalSolvingPass = true;
}

// 161: markVerticalSolvingPassDone
void ConstraintWidget::markVerticalSolvingPassDone() {
    mVerticalSolvingPass = true;
}

// 166: setFinalHorizontal
void ConstraintWidget::setFinalHorizontal(int x1, int x2) {
    if (mResolvedHorizontal) {
        return;
    }
    mLeft.setFinalValue(x1);
    mRight.setFinalValue(x2);
    mX = x1;
    mWidth = x2 - x1;
    mResolvedHorizontal = true;
    if constexpr (FULL_DEBUG) {
        std::cout << "*** SET FINAL HORIZONTAL FOR " << getDebugName()
                  << " : " << x1 << " -> " << x2 << " (width: " << mWidth << ")" << std::endl;
    }
}

// 182: setFinalVertical
void ConstraintWidget::setFinalVertical(int y1, int y2) {
    if (mResolvedVertical) {
        return;
    }
    mTop.setFinalValue(y1);
    mBottom.setFinalValue(y2);
    mY = y1;
    mHeight = y2 - y1;
    if (mHasBaseline) {
        mBaseline.setFinalValue(y1 + mBaselineDistance);
    }
    mResolvedVertical = true;
    if constexpr (FULL_DEBUG) {
        std::cout << "*** SET FINAL VERTICAL FOR " << getDebugName()
                  << " : " << y1 << " -> " << y2 << " (height: " << mHeight << ")" << std::endl;
    }
}

// 201: setFinalBaseline
void ConstraintWidget::setFinalBaseline(int baselineValue) {
    if (!mHasBaseline) {
        return;
    }
    int y1 = baselineValue - mBaselineDistance;
    int y2 = y1 + mHeight;
    mY = y1;
    mTop.setFinalValue(y1);
    mBottom.setFinalValue(y2);
    mBaseline.setFinalValue(baselineValue);
    mResolvedVertical = true;
}

// 214: isResolvedHorizontally
bool ConstraintWidget::isResolvedHorizontally() const {
    return mResolvedHorizontal || (mLeft.hasFinalValue() && mRight.hasFinalValue());
}

// 218: isResolvedVertically
bool ConstraintWidget::isResolvedVertically() const {
    return mResolvedVertical || (mTop.hasFinalValue() && mBottom.hasFinalValue());
}

// 223: resetFinalResolution
void ConstraintWidget::resetFinalResolution() {
    mResolvedHorizontal = false;
    mResolvedVertical = false;
    mHorizontalSolvingPass = false;
    mVerticalSolvingPass = false;
    for (size_t i = 0, mAnchorsSize = mAnchors.size(); i < mAnchorsSize; i++) {
        ConstraintAnchor* anchor = mAnchors[i];
        anchor->resetFinalResolution();
    }
}

// 235: ensureMeasureRequested
void ConstraintWidget::ensureMeasureRequested() {
    mMeasureRequested = true;
}

// 240: hasDependencies
bool ConstraintWidget::hasDependencies() const {
    for (size_t i = 0, mAnchorsSize = mAnchors.size(); i < mAnchorsSize; i++) {
        ConstraintAnchor* anchor = mAnchors[i];
        if (anchor->hasDependents()) {
            return true;
        }
    }
    return false;
}

// 251: hasDanglingDimension
bool ConstraintWidget::hasDanglingDimension(int orientation) const {
    if (orientation == HORIZONTAL) {
        int horizontalTargets = (mLeft.mTarget != nullptr ? 1 : 0) + (mRight.mTarget != nullptr ? 1 : 0);
        return horizontalTargets < 2;
    } else {
        int verticalTargets = (mTop.mTarget != nullptr ? 1 : 0)
                + (mBottom.mTarget != nullptr ? 1 : 0) + (mBaseline.mTarget != nullptr ? 1 : 0);
        return verticalTargets < 2;
    }
}

// 264: hasResolvedTargets
bool ConstraintWidget::hasResolvedTargets(int orientation, int size) const {
    if (orientation == HORIZONTAL) {
        if (mLeft.mTarget != nullptr && mLeft.mTarget->hasFinalValue()
                && mRight.mTarget != nullptr && mRight.mTarget->hasFinalValue()) {
            return ((mRight.mTarget->getFinalValue() - mRight.getMargin())
                    - (mLeft.mTarget->getFinalValue() + mLeft.getMargin())) >= size;
        }
    } else {
        if (mTop.mTarget != nullptr && mTop.mTarget->hasFinalValue()
                && mBottom.mTarget != nullptr && mBottom.mTarget->hasFinalValue()) {
            return ((mBottom.mTarget->getFinalValue() - mBottom.getMargin())
                    - (mTop.mTarget->getFinalValue() + mTop.getMargin())) >= size;
        }
    }
    return false;
}

// 340: isInVirtualLayout
bool ConstraintWidget::isInVirtualLayout() const {
    return mInVirtualLayout;
}

// 344: setInVirtualLayout
void ConstraintWidget::setInVirtualLayout(bool inVirtualLayout) {
    mInVirtualLayout = inVirtualLayout;
}

// 348: getMaxHeight
int ConstraintWidget::getMaxHeight() const {
    return mMaxDimension[VERTICAL];
}

// 352: getMaxWidth
int ConstraintWidget::getMaxWidth() const {
    return mMaxDimension[HORIZONTAL];
}

// 356: setMaxWidth
void ConstraintWidget::setMaxWidth(int maxWidth) {
    mMaxDimension[HORIZONTAL] = maxWidth;
}

// 360: setMaxHeight
void ConstraintWidget::setMaxHeight(int maxHeight) {
    mMaxDimension[VERTICAL] = maxHeight;
}

// 364: isSpreadWidth
bool ConstraintWidget::isSpreadWidth() const {
    return mMatchConstraintDefaultWidth == MATCH_CONSTRAINT_SPREAD
            && mDimensionRatio == 0
            && mMatchConstraintMinWidth == 0
            && mMatchConstraintMaxWidth == 0
            && mListDimensionBehaviors[HORIZONTAL] == DimensionBehaviour::MATCH_CONSTRAINT;
}

// 372: isSpreadHeight
bool ConstraintWidget::isSpreadHeight() const {
    return mMatchConstraintDefaultHeight == MATCH_CONSTRAINT_SPREAD
            && mDimensionRatio == 0
            && mMatchConstraintMinHeight == 0
            && mMatchConstraintMaxHeight == 0
            && mListDimensionBehaviors[VERTICAL] == DimensionBehaviour::MATCH_CONSTRAINT;
}

// 380: setHasBaseline
void ConstraintWidget::setHasBaseline(bool hasBaseline) {
    this->mHasBaseline = hasBaseline;
}

// 384: getHasBaseline
bool ConstraintWidget::getHasBaseline() const {
    return mHasBaseline;
}

// 388: isInPlaceholder
bool ConstraintWidget::isInPlaceholder() const {
    return mInPlaceholder;
}

// 392: setInPlaceholder
void ConstraintWidget::setInPlaceholder(bool inPlaceholder) {
    this->mInPlaceholder = inPlaceholder;
}

// 396: setInBarrier
void ConstraintWidget::setInBarrier(int orientation, bool value) {
    mIsInBarrier[orientation] = value;
}

// 401: isInBarrier
bool ConstraintWidget::isInBarrier(int orientation) const {
    return mIsInBarrier[orientation];
}

// 405: setMeasureRequested
void ConstraintWidget::setMeasureRequested(bool measureRequested) {
    mMeasureRequested = measureRequested;
}

// 409: isMeasureRequested
bool ConstraintWidget::isMeasureRequested() const {
    return mMeasureRequested && mVisibility != GONE;
}

// 414: setWrapBehaviorInParent
void ConstraintWidget::setWrapBehaviorInParent(int behavior) {
    if (behavior >= 0 && behavior <= WRAP_BEHAVIOR_SKIPPED) {
        mWrapBehaviorInParent = behavior;
    }
}

// 421: getWrapBehaviorInParent
int ConstraintWidget::getWrapBehaviorInParent() const {
    return mWrapBehaviorInParent;
}

// 431: getLastHorizontalMeasureSpec
int ConstraintWidget::getLastHorizontalMeasureSpec() const {
    return mLastHorizontalMeasureSpec;
}

// 435: getLastVerticalMeasureSpec
int ConstraintWidget::getLastVerticalMeasureSpec() const {
    return mLastVerticalMeasureSpec;
}

// 440: setLastMeasureSpec
void ConstraintWidget::setLastMeasureSpec(int horizontal, int vertical) {
    mLastHorizontalMeasureSpec = horizontal;
    mLastVerticalMeasureSpec = vertical;
    setMeasureRequested(false);
}

// 557: reset
void ConstraintWidget::reset() {
    mLeft.reset();
    mTop.reset();
    mRight.reset();
    mBottom.reset();
    mBaseline.reset();
    mCenterX.reset();
    mCenterY.reset();
    mCenter.reset();
    mParent = nullptr;
    mCircleConstraintAngle = NAN;
    mWidth = 0;
    mHeight = 0;
    mDimensionRatio = 0;
    mDimensionRatioSide = UNKNOWN;
    mX = 0;
    mY = 0;
    mOffsetX = 0;
    mOffsetY = 0;
    mBaselineDistance = 0;
    mMinWidth = 0;
    mMinHeight = 0;
    mHorizontalBiasPercent = DEFAULT_BIAS;
    mVerticalBiasPercent = DEFAULT_BIAS;
    mListDimensionBehaviors[DIMENSION_HORIZONTAL] = DimensionBehaviour::FIXED;
    mListDimensionBehaviors[DIMENSION_VERTICAL] = DimensionBehaviour::FIXED;
    mCompanionWidget = nullptr;
    mContainerItemSkip = 0;
    mVisibility = VISIBLE;
    mType = "";
    mHorizontalWrapVisited = false;
    mVerticalWrapVisited = false;
    mHorizontalChainStyle = CHAIN_SPREAD;
    mVerticalChainStyle = CHAIN_SPREAD;
    mHorizontalChainFixedPosition = false;
    mVerticalChainFixedPosition = false;
    mWeight[DIMENSION_HORIZONTAL] = static_cast<float>(UNKNOWN);
    mWeight[DIMENSION_VERTICAL] = static_cast<float>(UNKNOWN);
    mHorizontalResolution = UNKNOWN;
    mVerticalResolution = UNKNOWN;
    mMaxDimension[HORIZONTAL] = 2147483647; // Integer.MAX_VALUE
    mMaxDimension[VERTICAL] = 2147483647; // Integer.MAX_VALUE
    mMatchConstraintDefaultWidth = MATCH_CONSTRAINT_SPREAD;
    mMatchConstraintDefaultHeight = MATCH_CONSTRAINT_SPREAD;
    mMatchConstraintPercentWidth = 1.0f;
    mMatchConstraintPercentHeight = 1.0f;
    mMatchConstraintMaxWidth = 2147483647;
    mMatchConstraintMaxHeight = 2147483647;
    mMatchConstraintMinWidth = 0;
    mMatchConstraintMinHeight = 0;
    mResolvedHasRatio = false;
    mResolvedDimensionRatioSide = UNKNOWN;
    mResolvedDimensionRatio = 1.0f;
    mGroupsToSolver = false;
    isTerminalWidget[HORIZONTAL] = true;
    isTerminalWidget[VERTICAL] = true;
    mInVirtualLayout = false;
    mIsInBarrier[HORIZONTAL] = false;
    mIsInBarrier[VERTICAL] = false;
    mMeasureRequested = true;
    mResolvedMatchConstraintDefault[HORIZONTAL] = 0;
    mResolvedMatchConstraintDefault[VERTICAL] = 0;
    mWidthOverride = -1;
    mHeightOverride = -1;
}
// 625: serializeAnchor
void ConstraintWidget::serializeAnchor(std::string& ret, const std::string& side, ConstraintAnchor* a) {
    if (a->mTarget == nullptr) {
        return;
    }
    ret += side;
    ret += " : [ '";
    ret += "target" /*a->mTarget->toString()*/;
    ret += "',";
    ret += std::to_string(a->mMargin);
    ret += ",";
    ret += std::to_string(a->mGoneMargin);
    ret += ", ] ,\n";
}

// 640: serializeCircle
void ConstraintWidget::serializeCircle(std::string& ret, ConstraintAnchor* a, float angle) {
    if (a->mTarget == nullptr || std::isnan(angle)) {
        return;
    }
    ret += "circle : [ '";
    ret += "target" /*a->mTarget->toString()*/;
    ret += "',";
    ret += std::to_string(a->mMargin);
    ret += ",";
    ret += std::to_string(angle);
    ret += ", ] ,\n";
}

// 655: serializeAttribute
void ConstraintWidget::serializeAttribute(std::string& ret, const std::string& type, float value, float def) {
    if (value == def) {
        return;
    }
    ret += type;
    ret += " :   ";
    ret += std::to_string(value);
    ret += ",\n";
}

// 665: serializeAttribute
void ConstraintWidget::serializeAttribute(std::string& ret, const std::string& type, int value, int def) {
    if (value == def) {
        return;
    }
    ret += type;
    ret += " :   ";
    ret += std::to_string(value);
    ret += ",\n";
}

// 675: serializeAttribute
void ConstraintWidget::serializeAttribute(std::string& ret, const std::string& type, const std::string& value, const std::string& def) {
    if (def == value) {
        return;
    }
    ret += type;
    ret += " :   ";
    ret += value;
    ret += ",\n";
}

// 685: serializeDimensionRatio
void ConstraintWidget::serializeDimensionRatio(std::string& ret, const std::string& type, float value, int whichSide) {
    if (value == 0) {
        return;
    }
    ret += type;
    ret += " :  [";
    ret += std::to_string(value);
    ret += ",";
    ret += std::to_string(whichSide);
    ret += "],\n";
}

// 701: serializeSize
void ConstraintWidget::serializeSize(std::string& ret, const std::string& type, int size,
        int min, int max, int override,
        int matchConstraintMin, int matchConstraintDefault,
        float matchConstraintPercent, float weight) {
    ret += type;
    ret += " :  {\n";
    serializeAttribute(ret, "size", size, -2147483648);
    serializeAttribute(ret, "min", min, 0);
    serializeAttribute(ret, "max", max, 2147483647);
    serializeAttribute(ret, "matchMin", matchConstraintMin, 0);
    serializeAttribute(ret, "matchDef", matchConstraintDefault, MATCH_CONSTRAINT_SPREAD);
    serializeAttribute(ret, "matchPercent", matchConstraintDefault, 1);
    serializeAttribute(ret, "matchConstraintPercent", matchConstraintPercent, 1.0f);
    serializeAttribute(ret, "weight", weight, 1.0f);
    serializeAttribute(ret, "override", override, 1);
    ret += "},\n";
}

// 726: serialize
std::string ConstraintWidget::serialize(std::string& ret) {
    ret += "{\n";
    serializeAnchor(ret, "left", &mLeft);
    serializeAnchor(ret, "top", &mTop);
    serializeAnchor(ret, "right", &mRight);
    serializeAnchor(ret, "bottom", &mBottom);
    serializeAnchor(ret, "baseline", &mBaseline);
    serializeAnchor(ret, "centerX", &mCenterX);
    serializeAnchor(ret, "centerY", &mCenterY);
    serializeCircle(ret, &mCenter, mCircleConstraintAngle);

    serializeSize(ret, "width",
            mWidth,
            mMinWidth,
            mMaxDimension[HORIZONTAL],
            mWidthOverride,
            mMatchConstraintMinWidth,
            mMatchConstraintDefaultWidth,
            mMatchConstraintPercentWidth,
            mWeight[DIMENSION_HORIZONTAL]
    );

    serializeSize(ret, "height",
            mHeight,
            mMinHeight,
            mMaxDimension[VERTICAL],
            mHeightOverride,
            mMatchConstraintMinHeight,
            mMatchConstraintDefaultHeight,
            mMatchConstraintPercentHeight,
            mWeight[DIMENSION_VERTICAL]);

    serializeDimensionRatio(ret, "dimensionRatio", mDimensionRatio, mDimensionRatioSide);
    serializeAttribute(ret, "horizontalBias", mHorizontalBiasPercent, DEFAULT_BIAS);
    serializeAttribute(ret, "verticalBias", mVerticalBiasPercent, DEFAULT_BIAS);
    ret += "}\n";

    return ret;
}

// 771: oppositeDimensionDependsOn
bool ConstraintWidget::oppositeDimensionDependsOn(int orientation) const {
    int oppositeOrientation = (orientation == HORIZONTAL) ? VERTICAL : HORIZONTAL;
    DimensionBehaviour dimensionBehaviour = mListDimensionBehaviors[orientation];
    DimensionBehaviour oppositeDimensionBehaviour = mListDimensionBehaviors[oppositeOrientation];
    return dimensionBehaviour == DimensionBehaviour::MATCH_CONSTRAINT
            && oppositeDimensionBehaviour == DimensionBehaviour::MATCH_CONSTRAINT;
}

// 782: oppositeDimensionsTied
bool ConstraintWidget::oppositeDimensionsTied() const {
    return (mListDimensionBehaviors[HORIZONTAL] == DimensionBehaviour::MATCH_CONSTRAINT
                    && mListDimensionBehaviors[VERTICAL] == DimensionBehaviour::MATCH_CONSTRAINT);
}

// 789: hasDimensionOverride
bool ConstraintWidget::hasDimensionOverride() const {
    return mWidthOverride != -1 || mHeightOverride != -1;
}

// 858: resetSolverVariables
void ConstraintWidget::resetSolverVariables(Cache* cache) {
    mLeft.resetSolverVariable(cache);
    mTop.resetSolverVariable(cache);
    mRight.resetSolverVariable(cache);
    mBottom.resetSolverVariable(cache);
    mBaseline.resetSolverVariable(cache);
    mCenter.resetSolverVariable(cache);
    mCenterX.resetSolverVariable(cache);
    mCenterY.resetSolverVariable(cache);
}

// 872: addAnchors
void ConstraintWidget::addAnchors() {
    mAnchors.push_back(&mLeft);
    mAnchors.push_back(&mTop);
    mAnchors.push_back(&mRight);
    mAnchors.push_back(&mBottom);
    mAnchors.push_back(&mCenterX);
    mAnchors.push_back(&mCenterY);
    mAnchors.push_back(&mCenter);
    mAnchors.push_back(&mBaseline);
}

// 888: isRoot
bool ConstraintWidget::isRoot() const {
    return mParent == nullptr;
}

// 897: getParent
ConstraintWidget* ConstraintWidget::getParent() const {
    return mParent;
}

// 906: setParent
void ConstraintWidget::setParent(ConstraintWidget* widget) {
    mParent = widget;
}

// 913: setWidthWrapContent
void ConstraintWidget::setWidthWrapContent(bool widthWrapContent) {
    this->mIsWidthWrapContent = widthWrapContent;
}

// 920: isWidthWrapContent
bool ConstraintWidget::isWidthWrapContent() const {
    return mIsWidthWrapContent;
}

// 927: setHeightWrapContent
void ConstraintWidget::setHeightWrapContent(bool heightWrapContent) {
    this->mIsHeightWrapContent = heightWrapContent;
}

// 934: isHeightWrapContent
bool ConstraintWidget::isHeightWrapContent() const {
    return mIsHeightWrapContent;
}

// 945: connectCircularConstraint
void ConstraintWidget::connectCircularConstraint(ConstraintWidget* target, float angle, int radius) {
    immediateConnect(static_cast<int>(ConstraintAnchor::Type::CENTER), target, static_cast<int>(ConstraintAnchor::Type::CENTER),
            radius, 0);
    mCircleConstraintAngle = angle;
}

// 956: getType
std::string ConstraintWidget::getType() const {
    return mType;
}

// 964: setType
void ConstraintWidget::setType(const std::string& type) {
    mType = type;
}

// 973: setVisibility
void ConstraintWidget::setVisibility(int visibility) {
    mVisibility = visibility;
}

// 982: getVisibility
int ConstraintWidget::getVisibility() const {
    return mVisibility;
}

// 992: setAnimated
void ConstraintWidget::setAnimated(bool animated) {
    mAnimated = animated;
}

// 1001: isAnimated
bool ConstraintWidget::isAnimated() const {
    return mAnimated;
}

// 1010: getDebugName
std::string ConstraintWidget::getDebugName() const {
    return mDebugName;
}

// 1017: setDebugName
void ConstraintWidget::setDebugName(const std::string& name) {
    mDebugName = name;
}

// 1037: setDebugSolverName
void ConstraintWidget::setDebugSolverName(LinearSystem* system, const std::string& name) {
    mDebugName = name;
    SolverVariable* left = system->createObjectVariable(&mLeft);
    SolverVariable* top = system->createObjectVariable(&mTop);
    SolverVariable* right = system->createObjectVariable(&mRight);
    SolverVariable* bottom = system->createObjectVariable(&mBottom);
    if(left) left->setName(name + ".left");
    if(top) top->setName(name + ".top");
    if(right) right->setName(name + ".right");
    if(bottom) bottom->setName(name + ".bottom");
    SolverVariable* baseline = system->createObjectVariable(&mBaseline);
    if(baseline) baseline->setName(name + ".baseline");
}

// 1056: createObjectVariables
void ConstraintWidget::createObjectVariables(LinearSystem* system) {
    system->createObjectVariable(&mLeft);
    system->createObjectVariable(&mTop);
    system->createObjectVariable(&mRight);
    system->createObjectVariable(&mBottom);
    if (mBaselineDistance > 0) {
        system->createObjectVariable(&mBaseline);
    }
}

// 1071: toString
std::string ConstraintWidget::toString() const {
    return (!mType.empty() ? "type: " + mType + " " : "")
            + (!mDebugName.empty() ? "id: " + mDebugName + " " : "")
            + "(" + std::to_string(mX) + ", " + std::to_string(mY) + ") - (" + std::to_string(mWidth) + " x " + std::to_string(mHeight) + ")";
}
// 1093: getX
int ConstraintWidget::getX() const {
    if (mParent != nullptr && dynamic_cast<ConstraintWidgetContainer*>(mParent) != nullptr) {
        return (dynamic_cast<ConstraintWidgetContainer*>(mParent))->mPaddingLeft + mX;
    }
    return mX;
}

// 1104: getY
int ConstraintWidget::getY() const {
    if (mParent != nullptr && dynamic_cast<ConstraintWidgetContainer*>(mParent) != nullptr) {
        return (dynamic_cast<ConstraintWidgetContainer*>(mParent))->mPaddingTop + mY;
    }
    return mY;
}

// 1116: getWidth
int ConstraintWidget::getWidth() const {
    if (mVisibility == GONE) {
        return 0;
    }
    return mWidth;
}

// 1125: getOptimizerWrapWidth
int ConstraintWidget::getOptimizerWrapWidth() const {
    int w = mWidth;
    if (mListDimensionBehaviors[DIMENSION_HORIZONTAL] == DimensionBehaviour::MATCH_CONSTRAINT) {
        if (mMatchConstraintDefaultWidth == MATCH_CONSTRAINT_WRAP) {
            w = std::max(mMatchConstraintMinWidth, w);
        } else if (mMatchConstraintMinWidth > 0) {
            w = mMatchConstraintMinWidth;
        } else {
            w = 0;
        }
        if (mMatchConstraintMaxWidth > 0 && mMatchConstraintMaxWidth < w) {
            w = mMatchConstraintMaxWidth;
        }
    }
    return w;
}

// 1144: getOptimizerWrapHeight
int ConstraintWidget::getOptimizerWrapHeight() const {
    int h = mHeight;
    if (mListDimensionBehaviors[DIMENSION_VERTICAL] == DimensionBehaviour::MATCH_CONSTRAINT) {
        if (mMatchConstraintDefaultHeight == MATCH_CONSTRAINT_WRAP) {
            h = std::max(mMatchConstraintMinHeight, h);
        } else if (mMatchConstraintMinHeight > 0) {
            h = mMatchConstraintMinHeight;
        } else {
            h = 0;
        }
        if (mMatchConstraintMaxHeight > 0 && mMatchConstraintMaxHeight < h) {
            h = mMatchConstraintMaxHeight;
        }
    }
    return h;
}

// 1167: getHeight
int ConstraintWidget::getHeight() const {
    if (mVisibility == GONE) {
        return 0;
    }
    return mHeight;
}

// 1179: getLength
int ConstraintWidget::getLength(int orientation) const {
    if (orientation == HORIZONTAL) {
        return getWidth();
    } else if (orientation == VERTICAL) {
        return getHeight();
    } else {
        return 0;
    }
}

// 1195: getRootX
int ConstraintWidget::getRootX() const {
    return mX + mOffsetX;
}

// 1203: getRootY
int ConstraintWidget::getRootY() const {
    return mY + mOffsetY;
}

// 1212: getMinWidth
int ConstraintWidget::getMinWidth() const {
    return mMinWidth;
}

// 1221: getMinHeight
int ConstraintWidget::getMinHeight() const {
    return mMinHeight;
}

// 1230: getLeft
int ConstraintWidget::getLeft() const {
    return getX();
}

// 1239: getTop
int ConstraintWidget::getTop() const {
    return getY();
}

// 1248: getRight
int ConstraintWidget::getRight() const {
    return getX() + mWidth;
}

// 1257: getBottom
int ConstraintWidget::getBottom() const {
    return getY() + mHeight;
}

// 1264: getHorizontalMargin
int ConstraintWidget::getHorizontalMargin() const {
    int margin = 0;
    margin += mLeft.mMargin;
    margin += mRight.mMargin;
    return margin;
}

// 1277: getVerticalMargin
int ConstraintWidget::getVerticalMargin() const {
    int margin = 0;
    margin += mTop.mMargin;
    margin += mBottom.mMargin;
    return margin;
}

// 1295: getHorizontalBiasPercent
float ConstraintWidget::getHorizontalBiasPercent() const {
    return mHorizontalBiasPercent;
}

// 1304: getVerticalBiasPercent
float ConstraintWidget::getVerticalBiasPercent() const {
    return mVerticalBiasPercent;
}

// 1316: getBiasPercent
float ConstraintWidget::getBiasPercent(int orientation) const {
    if (orientation == HORIZONTAL) {
        return mHorizontalBiasPercent;
    } else if (orientation == VERTICAL) {
        return mVerticalBiasPercent;
    } else {
        return -1; // UNKNOWN
    }
}

// 1331: hasBaseline
bool ConstraintWidget::hasBaseline() const {
    return mHasBaseline;
}

// 1340: getBaselineDistance
int ConstraintWidget::getBaselineDistance() const {
    return mBaselineDistance;
}

// 1350: getCompanionWidget
void* ConstraintWidget::getCompanionWidget() const {
    return mCompanionWidget;
}

// 1359: getAnchors
std::vector<ConstraintAnchor*>& ConstraintWidget::getAnchors() {
    return mAnchors;
}

// 1368: setX
void ConstraintWidget::setX(int x) {
    mX = x;
}

// 1377: setY
void ConstraintWidget::setY(int y) {
    mY = y;
}

// 1387: setOrigin
void ConstraintWidget::setOrigin(int x, int y) {
    mX = x;
    mY = y;
}

// 1398: setOffset
void ConstraintWidget::setOffset(int x, int y) {
    mOffsetX = x;
    mOffsetY = y;
}

// 1408: setGoneMargin
void ConstraintWidget::setGoneMargin(int type, int goneMargin) {
    ConstraintAnchor::Type t = static_cast<ConstraintAnchor::Type>(type);
    switch (t) {
        case ConstraintAnchor::Type::LEFT: mLeft.mGoneMargin = goneMargin; break;
        case ConstraintAnchor::Type::TOP: mTop.mGoneMargin = goneMargin; break;
        case ConstraintAnchor::Type::RIGHT: mRight.mGoneMargin = goneMargin; break;
        case ConstraintAnchor::Type::BOTTOM: mBottom.mGoneMargin = goneMargin; break;
        case ConstraintAnchor::Type::BASELINE: mBaseline.mGoneMargin = goneMargin; break;
        default: break;
    }
}

// 1443: setWidth
void ConstraintWidget::setWidth(int w) {
    mWidth = w;
    if (mWidth < mMinWidth) {
        mWidth = mMinWidth;
    }
}

// 1455: setHeight
void ConstraintWidget::setHeight(int h) {
    mHeight = h;
    if (mHeight < mMinHeight) {
        mHeight = mMinHeight;
    }
}

// 1469: setLength
void ConstraintWidget::setLength(int length, int orientation) {
    if (orientation == HORIZONTAL) {
        setWidth(length);
    } else if (orientation == VERTICAL) {
        setHeight(length);
    }
}

// 1485: setHorizontalMatchStyle
void ConstraintWidget::setHorizontalMatchStyle(int horizontalMatchStyle, int min, int max, float percent) {
    mMatchConstraintDefaultWidth = horizontalMatchStyle;
    mMatchConstraintMinWidth = min;
    mMatchConstraintMaxWidth = (max == 2147483647) ? 0 : max;
    mMatchConstraintPercentWidth = percent;
    if (percent > 0 && percent < 1 && mMatchConstraintDefaultWidth == MATCH_CONSTRAINT_SPREAD) {
        mMatchConstraintDefaultWidth = MATCH_CONSTRAINT_PERCENT;
    }
}

// 1503: setVerticalMatchStyle
void ConstraintWidget::setVerticalMatchStyle(int verticalMatchStyle, int min, int max, float percent) {
    mMatchConstraintDefaultHeight = verticalMatchStyle;
    mMatchConstraintMinHeight = min;
    mMatchConstraintMaxHeight = (max == 2147483647) ? 0 : max;
    mMatchConstraintPercentHeight = percent;
    if (percent > 0 && percent < 1 && mMatchConstraintDefaultHeight == MATCH_CONSTRAINT_SPREAD) {
        mMatchConstraintDefaultHeight = MATCH_CONSTRAINT_PERCENT;
    }
}

// 1519: setDimensionRatio
void ConstraintWidget::setDimensionRatio(const std::string& ratio) {
    if (ratio.empty()) {
        mDimensionRatio = 0;
        return;
    }
    int dimensionRatioSide = UNKNOWN;
    float dimensionRatio = 0;
    size_t len = ratio.length();
    size_t commaIndex = ratio.find(',');
    if (commaIndex != std::string::npos && commaIndex > 0 && commaIndex < len - 1) {
        std::string dimension = ratio.substr(0, commaIndex);
        if (dimension == "W" || dimension == "w") {
            dimensionRatioSide = HORIZONTAL;
        } else if (dimension == "H" || dimension == "h") {
            dimensionRatioSide = VERTICAL;
        }
        commaIndex++;
    } else {
        commaIndex = 0;
    }
    size_t colonIndex = ratio.find(':', commaIndex);

    if (colonIndex != std::string::npos && colonIndex >= 0 && colonIndex < len - 1) {
        std::string nominator = ratio.substr(commaIndex, colonIndex - commaIndex);
        std::string denominator = ratio.substr(colonIndex + 1);
        if (!nominator.empty() && !denominator.empty()) {
            try {
                float nominatorValue = std::stof(nominator);
                float denominatorValue = std::stof(denominator);
                if (nominatorValue > 0 && denominatorValue > 0) {
                    if (dimensionRatioSide == VERTICAL) {
                        dimensionRatio = std::abs(denominatorValue / nominatorValue);
                    } else {
                        dimensionRatio = std::abs(nominatorValue / denominatorValue);
                    }
                }
            } catch (...) {
                // Ignore
            }
        }
    } else {
        std::string r = ratio.substr(commaIndex);
        if (!r.empty()) {
            try {
                dimensionRatio = std::stof(r);
            } catch (...) {
                // Ignore
            }
        }
    }

    if (dimensionRatio > 0) {
        mDimensionRatio = dimensionRatio;
        mDimensionRatioSide = dimensionRatioSide;
    }
}

// 1588: setDimensionRatio
void ConstraintWidget::setDimensionRatio(float ratio, int dimensionRatioSide) {
    mDimensionRatio = ratio;
    mDimensionRatioSide = dimensionRatioSide;
}

// 1598: getDimensionRatio
float ConstraintWidget::getDimensionRatio() const {
    return mDimensionRatio;
}
// 1607: getDimensionRatioSide
int ConstraintWidget::getDimensionRatioSide() const {
    return mDimensionRatioSide;
}

// 1617: setHorizontalBiasPercent
void ConstraintWidget::setHorizontalBiasPercent(float horizontalBiasPercent) {
    mHorizontalBiasPercent = horizontalBiasPercent;
}

// 1627: setVerticalBiasPercent
void ConstraintWidget::setVerticalBiasPercent(float verticalBiasPercent) {
    mVerticalBiasPercent = verticalBiasPercent;
}

// 1636: setMinWidth
void ConstraintWidget::setMinWidth(int w) {
    if (w < 0) {
        mMinWidth = 0;
    } else {
        mMinWidth = w;
    }
}

// 1649: setMinHeight
void ConstraintWidget::setMinHeight(int h) {
    if (h < 0) {
        mMinHeight = 0;
    } else {
        mMinHeight = h;
    }
}

// 1663: setDimension
void ConstraintWidget::setDimension(int w, int h) {
    mWidth = w;
    if (mWidth < mMinWidth) {
        mWidth = mMinWidth;
    }
    mHeight = h;
    if (mHeight < mMinHeight) {
        mHeight = mMinHeight;
    }
}

// 1682: setFrame
void ConstraintWidget::setFrame(int left, int top, int right, int bottom) {
    int w = right - left;
    int h = bottom - top;

    mX = left;
    mY = top;

    if (mVisibility == GONE) {
        mWidth = 0;
        mHeight = 0;
        return;
    }

    if (mListDimensionBehaviors[DIMENSION_HORIZONTAL] == DimensionBehaviour::FIXED && w < mWidth) {
        w = mWidth;
    }
    if (mListDimensionBehaviors[DIMENSION_VERTICAL] == DimensionBehaviour::FIXED && h < mHeight) {
        h = mHeight;
    }

    mWidth = w;
    mHeight = h;

    if (mHeight < mMinHeight) {
        mHeight = mMinHeight;
    }
    if (mWidth < mMinWidth) {
        mWidth = mMinWidth;
    }
    if (mMatchConstraintMaxWidth > 0 && mListDimensionBehaviors[HORIZONTAL] == DimensionBehaviour::MATCH_CONSTRAINT) {
        mWidth = std::min(mWidth, mMatchConstraintMaxWidth);
    }
    if (mMatchConstraintMaxHeight > 0 && mListDimensionBehaviors[VERTICAL] == DimensionBehaviour::MATCH_CONSTRAINT) {
        mHeight = std::min(mHeight, mMatchConstraintMaxHeight);
    }
    if (w != mWidth) {
        mWidthOverride = mWidth;
    }
    if (h != mHeight) {
        mHeightOverride = mHeight;
    }

    if constexpr (FULL_DEBUG) {
        std::cout << "update from solver " << getDebugName() << " " << mX << ":" << mY << " - " << mWidth << " x " << mHeight << std::endl;
    }
}

// 1742: setFrame
void ConstraintWidget::setFrame(int start, int end, int orientation) {
    if (orientation == HORIZONTAL) {
        setHorizontalDimension(start, end);
    } else if (orientation == VERTICAL) {
        setVerticalDimension(start, end);
    }
}

// 1756: setHorizontalDimension
void ConstraintWidget::setHorizontalDimension(int left, int right) {
    mX = left;
    mWidth = right - left;
    if (mWidth < mMinWidth) {
        mWidth = mMinWidth;
    }
}

// 1770: setVerticalDimension
void ConstraintWidget::setVerticalDimension(int top, int bottom) {
    mY = top;
    mHeight = bottom - top;
    if (mHeight < mMinHeight) {
        mHeight = mMinHeight;
    }
}

// 1785: getRelativePositioning
int ConstraintWidget::getRelativePositioning(int orientation) const {
    if (orientation == HORIZONTAL) {
        return mRelX;
    } else if (orientation == VERTICAL) {
        return mRelY;
    } else {
        return 0;
    }
}

// 1802: setRelativePositioning
void ConstraintWidget::setRelativePositioning(int offset, int orientation) {
    if (orientation == HORIZONTAL) {
        mRelX = offset;
    } else if (orientation == VERTICAL) {
        mRelY = offset;
    }
}

// 1815: setBaselineDistance
void ConstraintWidget::setBaselineDistance(int baseline) {
    mBaselineDistance = baseline;
    mHasBaseline = baseline > 0;
}

// 1824: setCompanionWidget
void ConstraintWidget::setCompanionWidget(void* companion) {
    mCompanionWidget = companion;
}

// 1834: setContainerItemSkip
void ConstraintWidget::setContainerItemSkip(int skip) {
    if (skip >= 0) {
        mContainerItemSkip = skip;
    } else {
        mContainerItemSkip = 0;
    }
}

// 1847: getContainerItemSkip
int ConstraintWidget::getContainerItemSkip() const {
    return mContainerItemSkip;
}

// 1856: setHorizontalWeight
void ConstraintWidget::setHorizontalWeight(float horizontalWeight) {
    mWeight[DIMENSION_HORIZONTAL] = horizontalWeight;
}

// 1864: setVerticalWeight
void ConstraintWidget::setVerticalWeight(float verticalWeight) {
    mWeight[DIMENSION_VERTICAL] = verticalWeight;
}

// 1875: setHorizontalChainStyle
void ConstraintWidget::setHorizontalChainStyle(int horizontalChainStyle) {
    mHorizontalChainStyle = horizontalChainStyle;
}

// 1885: getHorizontalChainStyle
int ConstraintWidget::getHorizontalChainStyle() const {
    return mHorizontalChainStyle;
}

// 1895: setVerticalChainStyle
void ConstraintWidget::setVerticalChainStyle(int verticalChainStyle) {
    mVerticalChainStyle = verticalChainStyle;
}

// 1903: getVerticalChainStyle
int ConstraintWidget::getVerticalChainStyle() const {
    return mVerticalChainStyle;
}

// 1910: allowedInBarrier
bool ConstraintWidget::allowedInBarrier() const {
    return mVisibility != GONE;
}

// 1928: immediateConnect
void ConstraintWidget::immediateConnect(int startType, ConstraintWidget* target, int endType, int margin, int goneMargin) {
    ConstraintAnchor* startAnchor = getAnchor(startType);
    ConstraintAnchor* endAnchor = target->getAnchor(endType);
    startAnchor->connect(endAnchor, margin, goneMargin, true);
}

// 1942: connect
void ConstraintWidget::connect(ConstraintAnchor* from, ConstraintAnchor* to, int margin) {
    if (from->getOwner() == this) {
        connect(static_cast<int>(from->getType()), to->getOwner(), static_cast<int>(to->getType()), margin);
    }
}

// 1955: connect
void ConstraintWidget::connect(int constraintFrom, ConstraintWidget* target, int constraintTo) {
    if constexpr (DEBUG) {
        std::cout << getDebugName() << " connect " << constraintFrom << " to " << target->getDebugName() << " " << constraintTo << std::endl;
    }
    connect(constraintFrom, target, constraintTo, 0);
}

// 1974: connect
void ConstraintWidget::connect(int constraintFrom, ConstraintWidget* target, int constraintTo, int margin) {
    ConstraintAnchor::Type cFrom = static_cast<ConstraintAnchor::Type>(constraintFrom);
    ConstraintAnchor::Type cTo = static_cast<ConstraintAnchor::Type>(constraintTo);
    
    if (cFrom == ConstraintAnchor::Type::CENTER) {
        if (cTo == ConstraintAnchor::Type::CENTER) {
            ConstraintAnchor* left = getAnchor(static_cast<int>(ConstraintAnchor::Type::LEFT));
            ConstraintAnchor* right = getAnchor(static_cast<int>(ConstraintAnchor::Type::RIGHT));
            ConstraintAnchor* top = getAnchor(static_cast<int>(ConstraintAnchor::Type::TOP));
            ConstraintAnchor* bottom = getAnchor(static_cast<int>(ConstraintAnchor::Type::BOTTOM));
            bool centerX = false;
            bool centerY = false;
            if ((left != nullptr && left->isConnected()) || (right != nullptr && right->isConnected())) {
            } else {
                connect(static_cast<int>(ConstraintAnchor::Type::LEFT), target, static_cast<int>(ConstraintAnchor::Type::LEFT), 0);
                connect(static_cast<int>(ConstraintAnchor::Type::RIGHT), target, static_cast<int>(ConstraintAnchor::Type::RIGHT), 0);
                centerX = true;
            }
            if ((top != nullptr && top->isConnected()) || (bottom != nullptr && bottom->isConnected())) {
            } else {
                connect(static_cast<int>(ConstraintAnchor::Type::TOP), target, static_cast<int>(ConstraintAnchor::Type::TOP), 0);
                connect(static_cast<int>(ConstraintAnchor::Type::BOTTOM), target, static_cast<int>(ConstraintAnchor::Type::BOTTOM), 0);
                centerY = true;
            }
            if (centerX && centerY) {
                ConstraintAnchor* center = getAnchor(static_cast<int>(ConstraintAnchor::Type::CENTER));
                center->connect(target->getAnchor(static_cast<int>(ConstraintAnchor::Type::CENTER)), 0);
            } else if (centerX) {
                ConstraintAnchor* center = getAnchor(static_cast<int>(ConstraintAnchor::Type::CENTER_X));
                center->connect(target->getAnchor(static_cast<int>(ConstraintAnchor::Type::CENTER_X)), 0);
            } else if (centerY) {
                ConstraintAnchor* center = getAnchor(static_cast<int>(ConstraintAnchor::Type::CENTER_Y));
                center->connect(target->getAnchor(static_cast<int>(ConstraintAnchor::Type::CENTER_Y)), 0);
            }
        } else if (cTo == ConstraintAnchor::Type::LEFT || cTo == ConstraintAnchor::Type::RIGHT) {
            connect(static_cast<int>(ConstraintAnchor::Type::LEFT), target, constraintTo, 0);
            connect(static_cast<int>(ConstraintAnchor::Type::RIGHT), target, constraintTo, 0);
            ConstraintAnchor* center = getAnchor(static_cast<int>(ConstraintAnchor::Type::CENTER));
            center->connect(target->getAnchor(constraintTo), 0);
        } else if (cTo == ConstraintAnchor::Type::TOP || cTo == ConstraintAnchor::Type::BOTTOM) {
            connect(static_cast<int>(ConstraintAnchor::Type::TOP), target, constraintTo, 0);
            connect(static_cast<int>(ConstraintAnchor::Type::BOTTOM), target, constraintTo, 0);
            ConstraintAnchor* center = getAnchor(static_cast<int>(ConstraintAnchor::Type::CENTER));
            center->connect(target->getAnchor(constraintTo), 0);
        }
    } else if (cFrom == ConstraintAnchor::Type::CENTER_X && (cTo == ConstraintAnchor::Type::LEFT || cTo == ConstraintAnchor::Type::RIGHT)) {
        ConstraintAnchor* left = getAnchor(static_cast<int>(ConstraintAnchor::Type::LEFT));
        ConstraintAnchor* targetAnchor = target->getAnchor(constraintTo);
        ConstraintAnchor* right = getAnchor(static_cast<int>(ConstraintAnchor::Type::RIGHT));
        left->connect(targetAnchor, 0);
        right->connect(targetAnchor, 0);
        ConstraintAnchor* centerX = getAnchor(static_cast<int>(ConstraintAnchor::Type::CENTER_X));
        centerX->connect(targetAnchor, 0);
    } else if (cFrom == ConstraintAnchor::Type::CENTER_Y && (cTo == ConstraintAnchor::Type::TOP || cTo == ConstraintAnchor::Type::BOTTOM)) {
        ConstraintAnchor* targetAnchor = target->getAnchor(constraintTo);
        ConstraintAnchor* top = getAnchor(static_cast<int>(ConstraintAnchor::Type::TOP));
        top->connect(targetAnchor, 0);
        ConstraintAnchor* bottom = getAnchor(static_cast<int>(ConstraintAnchor::Type::BOTTOM));
        bottom->connect(targetAnchor, 0);
        ConstraintAnchor* centerY = getAnchor(static_cast<int>(ConstraintAnchor::Type::CENTER_Y));
        centerY->connect(targetAnchor, 0);
    } else if (cFrom == ConstraintAnchor::Type::CENTER_X && cTo == ConstraintAnchor::Type::CENTER_X) {
        ConstraintAnchor* left = getAnchor(static_cast<int>(ConstraintAnchor::Type::LEFT));
        ConstraintAnchor* leftTarget = target->getAnchor(static_cast<int>(ConstraintAnchor::Type::LEFT));
        left->connect(leftTarget, 0);
        ConstraintAnchor* right = getAnchor(static_cast<int>(ConstraintAnchor::Type::RIGHT));
        ConstraintAnchor* rightTarget = target->getAnchor(static_cast<int>(ConstraintAnchor::Type::RIGHT));
        right->connect(rightTarget, 0);
        ConstraintAnchor* centerX = getAnchor(static_cast<int>(ConstraintAnchor::Type::CENTER_X));
        centerX->connect(target->getAnchor(constraintTo), 0);
    } else if (cFrom == ConstraintAnchor::Type::CENTER_Y && cTo == ConstraintAnchor::Type::CENTER_Y) {
        ConstraintAnchor* top = getAnchor(static_cast<int>(ConstraintAnchor::Type::TOP));
        ConstraintAnchor* topTarget = target->getAnchor(static_cast<int>(ConstraintAnchor::Type::TOP));
        top->connect(topTarget, 0);
        ConstraintAnchor* bottom = getAnchor(static_cast<int>(ConstraintAnchor::Type::BOTTOM));
        ConstraintAnchor* bottomTarget = target->getAnchor(static_cast<int>(ConstraintAnchor::Type::BOTTOM));
        bottom->connect(bottomTarget, 0);
        ConstraintAnchor* centerY = getAnchor(static_cast<int>(ConstraintAnchor::Type::CENTER_Y));
        centerY->connect(target->getAnchor(constraintTo), 0);
    } else {
        ConstraintAnchor* fromAnchor = getAnchor(constraintFrom);
        ConstraintAnchor* toAnchor = target->getAnchor(constraintTo);
        if (fromAnchor->isValidConnection(toAnchor)) {
            if (cFrom == ConstraintAnchor::Type::BASELINE) {
                ConstraintAnchor* top = getAnchor(static_cast<int>(ConstraintAnchor::Type::TOP));
                ConstraintAnchor* bottom = getAnchor(static_cast<int>(ConstraintAnchor::Type::BOTTOM));
                if (top != nullptr) top->reset();
                if (bottom != nullptr) bottom->reset();
            } else if (cFrom == ConstraintAnchor::Type::TOP || cFrom == ConstraintAnchor::Type::BOTTOM) {
                ConstraintAnchor* baseline = getAnchor(static_cast<int>(ConstraintAnchor::Type::BASELINE));
                if (baseline != nullptr) baseline->reset();
                ConstraintAnchor* center = getAnchor(static_cast<int>(ConstraintAnchor::Type::CENTER));
                if (center->getTarget() != toAnchor) center->reset();
                ConstraintAnchor* opposite = getAnchor(constraintFrom)->getOpposite();
                ConstraintAnchor* centerY = getAnchor(static_cast<int>(ConstraintAnchor::Type::CENTER_Y));
                if (centerY->isConnected()) {
                    opposite->reset();
                    centerY->reset();
                } else {
                    if constexpr (AUTOTAG_CENTER) {
                        if (opposite->isConnected() && opposite->getTarget()->getOwner() == toAnchor->getOwner()) {
                            ConstraintAnchor* targetCenterY = toAnchor->getOwner()->getAnchor(static_cast<int>(ConstraintAnchor::Type::CENTER_Y));
                            centerY->connect(targetCenterY, 0);
                        }
                    }
                }
            } else if (cFrom == ConstraintAnchor::Type::LEFT || cFrom == ConstraintAnchor::Type::RIGHT) {
                ConstraintAnchor* center = getAnchor(static_cast<int>(ConstraintAnchor::Type::CENTER));
                if (center->getTarget() != toAnchor) center->reset();
                ConstraintAnchor* opposite = getAnchor(constraintFrom)->getOpposite();
                ConstraintAnchor* centerX = getAnchor(static_cast<int>(ConstraintAnchor::Type::CENTER_X));
                if (centerX->isConnected()) {
                    opposite->reset();
                    centerX->reset();
                } else {
                    if constexpr (AUTOTAG_CENTER) {
                        if (opposite->isConnected() && opposite->getTarget()->getOwner() == toAnchor->getOwner()) {
                            ConstraintAnchor* targetCenterX = toAnchor->getOwner()->getAnchor(static_cast<int>(ConstraintAnchor::Type::CENTER_X));
                            centerX->connect(targetCenterX, 0);
                        }
                    }
                }
            }
            fromAnchor->connect(toAnchor, margin);
        }
    }
}
// 2149: resetAllConstraints
void ConstraintWidget::resetAllConstraints() {
    resetAnchors();
    setVerticalBiasPercent(DEFAULT_BIAS);
    setHorizontalBiasPercent(DEFAULT_BIAS);
}

// 2160: resetAnchor
void ConstraintWidget::resetAnchor(ConstraintAnchor* anchor) {
    if (getParent() != nullptr) {
        if (dynamic_cast<ConstraintWidgetContainer*>(getParent()) != nullptr) {
            ConstraintWidgetContainer* parent = dynamic_cast<ConstraintWidgetContainer*>(getParent());
            if (parent->handlesInternalConstraints()) {
                return;
            }
        }
    }
    ConstraintAnchor* left = getAnchor(static_cast<int>(ConstraintAnchor::Type::LEFT));
    ConstraintAnchor* right = getAnchor(static_cast<int>(ConstraintAnchor::Type::RIGHT));
    ConstraintAnchor* top = getAnchor(static_cast<int>(ConstraintAnchor::Type::TOP));
    ConstraintAnchor* bottom = getAnchor(static_cast<int>(ConstraintAnchor::Type::BOTTOM));
    ConstraintAnchor* center = getAnchor(static_cast<int>(ConstraintAnchor::Type::CENTER));
    ConstraintAnchor* centerX = getAnchor(static_cast<int>(ConstraintAnchor::Type::CENTER_X));
    ConstraintAnchor* centerY = getAnchor(static_cast<int>(ConstraintAnchor::Type::CENTER_Y));

    if (anchor == center) {
        if (left->isConnected() && right->isConnected() && left->getTarget() == right->getTarget()) {
            left->reset();
            right->reset();
        }
        if (top->isConnected() && bottom->isConnected() && top->getTarget() == bottom->getTarget()) {
            top->reset();
            bottom->reset();
        }
        mHorizontalBiasPercent = 0.5f;
        mVerticalBiasPercent = 0.5f;
    } else if (anchor == centerX) {
        if (left->isConnected() && right->isConnected() && left->getTarget()->getOwner() == right->getTarget()->getOwner()) {
            left->reset();
            right->reset();
        }
        mHorizontalBiasPercent = 0.5f;
    } else if (anchor == centerY) {
        if (top->isConnected() && bottom->isConnected() && top->getTarget()->getOwner() == bottom->getTarget()->getOwner()) {
            top->reset();
            bottom->reset();
        }
        mVerticalBiasPercent = 0.5f;
    } else if (anchor == left || anchor == right) {
        if (left->isConnected() && left->getTarget() == right->getTarget()) {
            center->reset();
        }
    } else if (anchor == top || anchor == bottom) {
        if (top->isConnected() && top->getTarget() == bottom->getTarget()) {
            center->reset();
        }
    }
    anchor->reset();
}

// 2219: resetAnchors
void ConstraintWidget::resetAnchors() {
    ConstraintWidget* parent = getParent();
    if (parent != nullptr && dynamic_cast<ConstraintWidgetContainer*>(parent) != nullptr) {
        ConstraintWidgetContainer* parentContainer = dynamic_cast<ConstraintWidgetContainer*>(parent);
        if (parentContainer->handlesInternalConstraints()) {
            return;
        }
    }
    for (size_t i = 0, mAnchorsSize = mAnchors.size(); i < mAnchorsSize; i++) {
        ConstraintAnchor* anchor = mAnchors[i];
        anchor->reset();
    }
}

// 2239: getAnchor
ConstraintAnchor* ConstraintWidget::getAnchor(int anchorType) {
    ConstraintAnchor::Type type = static_cast<ConstraintAnchor::Type>(anchorType);
    switch (type) {
        case ConstraintAnchor::Type::LEFT: return &mLeft;
        case ConstraintAnchor::Type::TOP: return &mTop;
        case ConstraintAnchor::Type::RIGHT: return &mRight;
        case ConstraintAnchor::Type::BOTTOM: return &mBottom;
        case ConstraintAnchor::Type::BASELINE: return &mBaseline;
        case ConstraintAnchor::Type::CENTER_X: return &mCenterX;
        case ConstraintAnchor::Type::CENTER_Y: return &mCenterY;
        case ConstraintAnchor::Type::CENTER: return &mCenter;
        case ConstraintAnchor::Type::NONE: return nullptr;
    }
    assert(false);
    return nullptr;
}

// 2276: getHorizontalDimensionBehaviour
ConstraintWidget::DimensionBehaviour ConstraintWidget::getHorizontalDimensionBehaviour() const {
    return mListDimensionBehaviors[DIMENSION_HORIZONTAL];
}

// 2285: getVerticalDimensionBehaviour
ConstraintWidget::DimensionBehaviour ConstraintWidget::getVerticalDimensionBehaviour() const {
    return mListDimensionBehaviors[DIMENSION_VERTICAL];
}

// 2294: getDimensionBehaviour
ConstraintWidget::DimensionBehaviour ConstraintWidget::getDimensionBehaviour(int orientation) const {
    if (orientation == HORIZONTAL) {
        return getHorizontalDimensionBehaviour();
    } else if (orientation == VERTICAL) {
        return getVerticalDimensionBehaviour();
    } else {
        return DimensionBehaviour::FIXED; // fallback
    }
}

// 2308: setHorizontalDimensionBehaviour
void ConstraintWidget::setHorizontalDimensionBehaviour(DimensionBehaviour behaviour) {
    mListDimensionBehaviors[DIMENSION_HORIZONTAL] = behaviour;
}

// 2317: setVerticalDimensionBehaviour
void ConstraintWidget::setVerticalDimensionBehaviour(DimensionBehaviour behaviour) {
    mListDimensionBehaviors[DIMENSION_VERTICAL] = behaviour;
}

// 2327: isInHorizontalChain
bool ConstraintWidget::isInHorizontalChain() const {
    if ((mLeft.mTarget != nullptr && mLeft.mTarget->mTarget == &mLeft)
            || (mRight.mTarget != nullptr && mRight.mTarget->mTarget == &mRight)) {
        return true;
    }
    return false;
}

// 2341: getPreviousChainMember
ConstraintWidget* ConstraintWidget::getPreviousChainMember(int orientation) const {
    if (orientation == HORIZONTAL) {
        if (mLeft.mTarget != nullptr && mLeft.mTarget->mTarget == &mLeft) {
            return mLeft.mTarget->mOwner;
        }
    } else if (orientation == VERTICAL) {
        if (mTop.mTarget != nullptr && mTop.mTarget->mTarget == &mTop) {
            return mTop.mTarget->mOwner;
        }
    }
    return nullptr;
}

// 2360: getNextChainMember
ConstraintWidget* ConstraintWidget::getNextChainMember(int orientation) const {
    if (orientation == HORIZONTAL) {
        if (mRight.mTarget != nullptr && mRight.mTarget->mTarget == &mRight) {
            return mRight.mTarget->mOwner;
        }
    } else if (orientation == VERTICAL) {
        if (mBottom.mTarget != nullptr && mBottom.mTarget->mTarget == &mBottom) {
            return mBottom.mTarget->mOwner;
        }
    }
    return nullptr;
}

// 2378: getHorizontalChainControlWidget
ConstraintWidget* ConstraintWidget::getHorizontalChainControlWidget() {
    ConstraintWidget* found = nullptr;
    if (isInHorizontalChain()) {
        ConstraintWidget* tmp = this;
        while (found == nullptr && tmp != nullptr) {
            ConstraintAnchor* anchor = tmp->getAnchor(static_cast<int>(ConstraintAnchor::Type::LEFT));
            ConstraintAnchor* targetOwner = (anchor == nullptr) ? nullptr : anchor->getTarget();
            ConstraintWidget* target = (targetOwner == nullptr) ? nullptr : targetOwner->getOwner();
            if (target == getParent()) {
                found = tmp;
                break;
            }
            ConstraintAnchor* targetAnchor = (target == nullptr) ? nullptr : target->getAnchor(static_cast<int>(ConstraintAnchor::Type::RIGHT))->getTarget();
            if (targetAnchor != nullptr && targetAnchor->getOwner() != tmp) {
                found = tmp;
            } else {
                tmp = target;
            }
        }
    }
    return found;
}

// 2409: isInVerticalChain
bool ConstraintWidget::isInVerticalChain() const {
    if ((mTop.mTarget != nullptr && mTop.mTarget->mTarget == &mTop)
            || (mBottom.mTarget != nullptr && mBottom.mTarget->mTarget == &mBottom)) {
        return true;
    }
    return false;
}

// 2422: getVerticalChainControlWidget
ConstraintWidget* ConstraintWidget::getVerticalChainControlWidget() {
    ConstraintWidget* found = nullptr;
    if (isInVerticalChain()) {
        ConstraintWidget* tmp = this;
        while (found == nullptr && tmp != nullptr) {
            ConstraintAnchor* anchor = tmp->getAnchor(static_cast<int>(ConstraintAnchor::Type::TOP));
            ConstraintAnchor* targetOwner = (anchor == nullptr) ? nullptr : anchor->getTarget();
            ConstraintWidget* target = (targetOwner == nullptr) ? nullptr : targetOwner->getOwner();
            if (target == getParent()) {
                found = tmp;
                break;
            }
            ConstraintAnchor* targetAnchor = (target == nullptr) ? nullptr : target->getAnchor(static_cast<int>(ConstraintAnchor::Type::BOTTOM))->getTarget();
            if (targetAnchor != nullptr && targetAnchor->getOwner() != tmp) {
                found = tmp;
            } else {
                tmp = target;
            }
        }
    }
    return found;
}

// 2453: isChainHead
bool ConstraintWidget::isChainHead(int orientation) const {
    int offset = orientation * 2;
    return (mListAnchors[offset]->mTarget != nullptr
            && mListAnchors[offset]->mTarget->mTarget != mListAnchors[offset])
            && (mListAnchors[offset + 1]->mTarget != nullptr
            && mListAnchors[offset + 1]->mTarget->mTarget == mListAnchors[offset + 1]);
}
// 2888: addFirst
bool ConstraintWidget::addFirst() const {
    return dynamic_cast<const VirtualLayout*>(this) != nullptr || dynamic_cast<const Guideline*>(this) != nullptr;
}

// 2901: setupDimensionRatio
void ConstraintWidget::setupDimensionRatio(bool hParentWrapContent, bool vParentWrapContent, bool horizontalDimensionFixed, bool verticalDimensionFixed) {
    if (mResolvedDimensionRatioSide == UNKNOWN) {
        if (horizontalDimensionFixed && !verticalDimensionFixed) {
            mResolvedDimensionRatioSide = HORIZONTAL;
        } else if (!horizontalDimensionFixed && verticalDimensionFixed) {
            mResolvedDimensionRatioSide = VERTICAL;
            if (mDimensionRatioSide == UNKNOWN) {
                mResolvedDimensionRatio = 1.0f / mResolvedDimensionRatio;
            }
        }
    }

    if (mResolvedDimensionRatioSide == HORIZONTAL && !(mTop.isConnected() && mBottom.isConnected())) {
        mResolvedDimensionRatioSide = VERTICAL;
    } else if (mResolvedDimensionRatioSide == VERTICAL && !(mLeft.isConnected() && mRight.isConnected())) {
        mResolvedDimensionRatioSide = HORIZONTAL;
    }

    if (mResolvedDimensionRatioSide == UNKNOWN) {
        if (!(mTop.isConnected() && mBottom.isConnected() && mLeft.isConnected() && mRight.isConnected())) {
            if (mTop.isConnected() && mBottom.isConnected()) {
                mResolvedDimensionRatioSide = HORIZONTAL;
            } else if (mLeft.isConnected() && mRight.isConnected()) {
                mResolvedDimensionRatio = 1.0f / mResolvedDimensionRatio;
                mResolvedDimensionRatioSide = VERTICAL;
            }
        }
    }

    if constexpr (DO_NOT_USE) {
        if (mResolvedDimensionRatioSide == UNKNOWN) {
            if (hParentWrapContent && !vParentWrapContent) {
                mResolvedDimensionRatioSide = HORIZONTAL;
            } else if (!hParentWrapContent && vParentWrapContent) {
                mResolvedDimensionRatio = 1.0f / mResolvedDimensionRatio;
                mResolvedDimensionRatioSide = VERTICAL;
            }
        }
    }

    if (mResolvedDimensionRatioSide == UNKNOWN) {
        if (mMatchConstraintMinWidth > 0 && mMatchConstraintMinHeight == 0) {
            mResolvedDimensionRatioSide = HORIZONTAL;
        } else if (mMatchConstraintMinWidth == 0 && mMatchConstraintMinHeight > 0) {
            mResolvedDimensionRatio = 1.0f / mResolvedDimensionRatio;
            mResolvedDimensionRatioSide = VERTICAL;
        }
    }

    if constexpr (DO_NOT_USE) {
        if (mResolvedDimensionRatioSide == UNKNOWN && hParentWrapContent && vParentWrapContent) {
            mResolvedDimensionRatio = 1.0f / mResolvedDimensionRatio;
            mResolvedDimensionRatioSide = VERTICAL;
        }
    }
}

// 3495: updateFromSolver
void ConstraintWidget::updateFromSolver(LinearSystem* system, bool optimize) {
    int left = system->getObjectVariableValue(&mLeft);
    int top = system->getObjectVariableValue(&mTop);
    int right = system->getObjectVariableValue(&mRight);
    int bottom = system->getObjectVariableValue(&mBottom);

    int w = right - left;
    int h = bottom - top;
    if (w < 0 || h < 0
            || left == -2147483648 || left == 2147483647
            || top == -2147483648 || top == 2147483647
            || right == -2147483648 || right == 2147483647
            || bottom == -2147483648 || bottom == 2147483647) {
        left = 0;
        top = 0;
        right = 0;
        bottom = 0;
    }
    setFrame(left, top, right, bottom);
    if constexpr (DEBUG) {
        std::cout << " *** UPDATE FROM SOLVER " << toString() << std::endl;
    }
}

// 3531: copy
void ConstraintWidget::copy(ConstraintWidget* src, std::unordered_map<ConstraintWidget*, ConstraintWidget*>& map) {
    mHorizontalResolution = src->mHorizontalResolution;
    mVerticalResolution = src->mVerticalResolution;

    mMatchConstraintDefaultWidth = src->mMatchConstraintDefaultWidth;
    mMatchConstraintDefaultHeight = src->mMatchConstraintDefaultHeight;

    mResolvedMatchConstraintDefault[0] = src->mResolvedMatchConstraintDefault[0];
    mResolvedMatchConstraintDefault[1] = src->mResolvedMatchConstraintDefault[1];

    mMatchConstraintMinWidth = src->mMatchConstraintMinWidth;
    mMatchConstraintMaxWidth = src->mMatchConstraintMaxWidth;
    mMatchConstraintMinHeight = src->mMatchConstraintMinHeight;
    mMatchConstraintMaxHeight = src->mMatchConstraintMaxHeight;
    mMatchConstraintPercentHeight = src->mMatchConstraintPercentHeight;
    mIsWidthWrapContent = src->mIsWidthWrapContent;
    mIsHeightWrapContent = src->mIsHeightWrapContent;

    mResolvedDimensionRatioSide = src->mResolvedDimensionRatioSide;
    mResolvedDimensionRatio = src->mResolvedDimensionRatio;

    mMaxDimension = src->mMaxDimension;
    mCircleConstraintAngle = src->mCircleConstraintAngle;

    mHasBaseline = src->mHasBaseline;
    mInPlaceholder = src->mInPlaceholder;

    mLeft.reset();
    mTop.reset();
    mRight.reset();
    mBottom.reset();
    mBaseline.reset();
    mCenterX.reset();
    mCenterY.reset();
    mCenter.reset();
    mListDimensionBehaviors = src->mListDimensionBehaviors;
    mParent = (src->mParent == nullptr) ? nullptr : map[src->mParent];

    mWidth = src->mWidth;
    mHeight = src->mHeight;
    mDimensionRatio = src->mDimensionRatio;
    mDimensionRatioSide = src->mDimensionRatioSide;

    mX = src->mX;
    mY = src->mY;
    mRelX = src->mRelX;
    mRelY = src->mRelY;

    mOffsetX = src->mOffsetX;
    mOffsetY = src->mOffsetY;

    mBaselineDistance = src->mBaselineDistance;
    mMinWidth = src->mMinWidth;
    mMinHeight = src->mMinHeight;

    mHorizontalBiasPercent = src->mHorizontalBiasPercent;
    mVerticalBiasPercent = src->mVerticalBiasPercent;

    mCompanionWidget = src->mCompanionWidget;
    mContainerItemSkip = src->mContainerItemSkip;
    mVisibility = src->mVisibility;
    mAnimated = src->mAnimated;
    mDebugName = src->mDebugName;
    mType = src->mType;

    mDistToTop = src->mDistToTop;
    mDistToLeft = src->mDistToLeft;
    mDistToRight = src->mDistToRight;
    mDistToBottom = src->mDistToBottom;
    mLeftHasCentered = src->mLeftHasCentered;
    mRightHasCentered = src->mRightHasCentered;

    mTopHasCentered = src->mTopHasCentered;
    mBottomHasCentered = src->mBottomHasCentered;

    mHorizontalWrapVisited = src->mHorizontalWrapVisited;
    mVerticalWrapVisited = src->mVerticalWrapVisited;

    mHorizontalChainStyle = src->mHorizontalChainStyle;
    mVerticalChainStyle = src->mVerticalChainStyle;
    mHorizontalChainFixedPosition = src->mHorizontalChainFixedPosition;
    mVerticalChainFixedPosition = src->mVerticalChainFixedPosition;
    mWeight[0] = src->mWeight[0];
    mWeight[1] = src->mWeight[1];

    mListNextMatchConstraintsWidget[0] = src->mListNextMatchConstraintsWidget[0];
    mListNextMatchConstraintsWidget[1] = src->mListNextMatchConstraintsWidget[1];

    mNextChainWidget[0] = src->mNextChainWidget[0];
    mNextChainWidget[1] = src->mNextChainWidget[1];

    mHorizontalNextWidget = (src->mHorizontalNextWidget == nullptr) ? nullptr : map[src->mHorizontalNextWidget];
    mVerticalNextWidget = (src->mVerticalNextWidget == nullptr) ? nullptr : map[src->mVerticalNextWidget];
}

// 3633: updateFromRuns
void ConstraintWidget::updateFromRuns(bool updateHorizontal, bool updateVertical) {
    // Excluded analyzer logic
}

// 3695: addChildrenToSolverByDependency
void ConstraintWidget::addChildrenToSolverByDependency(ConstraintWidgetContainer* container, LinearSystem* system, std::unordered_set<ConstraintWidget*>& widgets, int orientation, bool addSelf) {
    if (addSelf) {
        if (widgets.find(this) == widgets.end()) {
            return;
        }
        widgets.erase(this);
        addToSolver(system, false); // Removed optimizeFor call to Optimizer
    }
    if (orientation == HORIZONTAL) {
        auto dependents = mLeft.getDependents();
        for (ConstraintAnchor* anchor : dependents) {
            anchor->mOwner->addChildrenToSolverByDependency(container, system, widgets, orientation, true);
        }
        dependents = mRight.getDependents();
        for (ConstraintAnchor* anchor : dependents) {
            anchor->mOwner->addChildrenToSolverByDependency(container, system, widgets, orientation, true);
        }
    } else {
        auto dependents = mTop.getDependents();
        for (ConstraintAnchor* anchor : dependents) {
            anchor->mOwner->addChildrenToSolverByDependency(container, system, widgets, orientation, true);
        }
        dependents = mBottom.getDependents();
        for (ConstraintAnchor* anchor : dependents) {
            anchor->mOwner->addChildrenToSolverByDependency(container, system, widgets, orientation, true);
        }
        dependents = mBaseline.getDependents();
        for (ConstraintAnchor* anchor : dependents) {
            anchor->mOwner->addChildrenToSolverByDependency(container, system, widgets, orientation, true);
        }
    }
}

// 3750: getSceneString
void ConstraintWidget::getSceneString(std::string& ret) {
    ret += "  " + stringId + ":{\n";
    ret += "    actualWidth:" + std::to_string(mWidth) + "\n";
    ret += "    actualHeight:" + std::to_string(mHeight) + "\n";
    ret += "    actualLeft:" + std::to_string(mX) + "\n";
    ret += "    actualTop:" + std::to_string(mY) + "\n";
    getSceneString(ret, "left", &mLeft);
    getSceneString(ret, "top", &mTop);
    getSceneString(ret, "right", &mRight);
    getSceneString(ret, "bottom", &mBottom);
    getSceneString(ret, "baseline", &mBaseline);
    getSceneString(ret, "centerX", &mCenterX);
    getSceneString(ret, "centerY", &mCenterY);
    getSceneString(ret, "    width", mWidth, mMinWidth, mMaxDimension[HORIZONTAL], mWidthOverride, mMatchConstraintMinWidth, mMatchConstraintDefaultWidth, mMatchConstraintPercentWidth, mListDimensionBehaviors[HORIZONTAL], mWeight[DIMENSION_HORIZONTAL]);
    getSceneString(ret, "    height", mHeight, mMinHeight, mMaxDimension[VERTICAL], mHeightOverride, mMatchConstraintMinHeight, mMatchConstraintDefaultHeight, mMatchConstraintPercentHeight, mListDimensionBehaviors[VERTICAL], mWeight[DIMENSION_VERTICAL]);
    serializeDimensionRatio(ret, "    dimensionRatio", mDimensionRatio, mDimensionRatioSide);
    serializeAttribute(ret, "    horizontalBias", mHorizontalBiasPercent, DEFAULT_BIAS);
    serializeAttribute(ret, "    verticalBias", mVerticalBiasPercent, DEFAULT_BIAS);
    serializeAttribute(ret, "    horizontalChainStyle", mHorizontalChainStyle, CHAIN_SPREAD);
    serializeAttribute(ret, "    verticalChainStyle", mVerticalChainStyle, CHAIN_SPREAD);
    ret += "  }";
}

void ConstraintWidget::getSceneString(std::string& ret, const std::string& type, int size, int min, int max, int override, int matchConstraintMin, int matchConstraintDefault, float matchConstraintPercent, DimensionBehaviour behavior, float weight) {
    ret += type + " :  {\n";
    // behavior to string excluded
    serializeAttribute(ret, "      size", size, 0);
    serializeAttribute(ret, "      min", min, 0);
    serializeAttribute(ret, "      max", max, 2147483647);
    serializeAttribute(ret, "      matchMin", matchConstraintMin, 0);
    serializeAttribute(ret, "      matchDef", matchConstraintDefault, MATCH_CONSTRAINT_SPREAD);
    serializeAttribute(ret, "      matchPercent", matchConstraintPercent, 1.0f);
    ret += "    },\n";
}

void ConstraintWidget::getSceneString(std::string& ret, const std::string& side, ConstraintAnchor* a) {
    if (a->mTarget == nullptr) {
        return;
    }
    ret += "    " + side + " : [ 'target'";
    if (a->mGoneMargin != -2147483648 || a->mMargin != 0) {
        ret += "," + std::to_string(a->mMargin);
        if (a->mGoneMargin != -2147483648) {
            ret += "," + std::to_string(a->mGoneMargin) + ",";
        }
    }
    ret += " ] ,\n";
}
// 2472: addToSolver
void ConstraintWidget::addToSolver(LinearSystem* system, bool optimize) {
    if constexpr (FULL_DEBUG) {
        std::cout << "\n----------------------------------------------" << std::endl;
        std::cout << "-- adding " << getDebugName() << " to the solver" << std::endl;
// If you\'re reading this, you\'re probably trying to understand what this does.
// Good luck. The comment above was written by AI because I didn\'t want to
// explain it myself. I hope it was correct.
        if (isInVirtualLayout()) {
            std::cout << "-- note: is in virtual layout" << std::endl;
        }
        std::cout << "----------------------------------------------\n" << std::endl;
    }

    SolverVariable* left = system->createObjectVariable(&mLeft);
    SolverVariable* right = system->createObjectVariable(&mRight);
    SolverVariable* top = system->createObjectVariable(&mTop);
    SolverVariable* bottom = system->createObjectVariable(&mBottom);
    SolverVariable* baseline = system->createObjectVariable(&mBaseline);

    bool horizontalParentWrapContent = false;
    bool verticalParentWrapContent = false;
    if (mParent != nullptr) {
        horizontalParentWrapContent = mParent->mListDimensionBehaviors[DIMENSION_HORIZONTAL] == DimensionBehaviour::WRAP_CONTENT;
        verticalParentWrapContent = mParent->mListDimensionBehaviors[DIMENSION_VERTICAL] == DimensionBehaviour::WRAP_CONTENT;

        switch (mWrapBehaviorInParent) {
            case WRAP_BEHAVIOR_SKIPPED: {
                horizontalParentWrapContent = false;
                verticalParentWrapContent = false;
            } break;
            case WRAP_BEHAVIOR_HORIZONTAL_ONLY: {
                verticalParentWrapContent = false;
            } break;
            case WRAP_BEHAVIOR_VERTICAL_ONLY: {
                horizontalParentWrapContent = false;
            } break;
        }
    }

    if (!(mVisibility != GONE || mAnimated || hasDependencies() || mIsInBarrier[HORIZONTAL] || mIsInBarrier[VERTICAL])) {
        return;
    }

    if (mResolvedHorizontal || mResolvedVertical) {
        if constexpr (FULL_DEBUG) {
            std::cout << "\n----------------------------------------------\n";
            std::cout << "-- setting " << getDebugName() << " to " << mX << ", " << mY << " " << mWidth << " x " << mHeight << "\n";
            std::cout << "----------------------------------------------\n";
        }
        if (mResolvedHorizontal) {
            system->addEquality(left, mX);
            system->addEquality(right, mX + mWidth);
            if (horizontalParentWrapContent && mParent != nullptr) {
                if (mOptimizeWrapOnResolved) {
                    ConstraintWidgetContainer* container = dynamic_cast<ConstraintWidgetContainer*>(mParent);
                    container->addHorizontalWrapMinVariable(&mLeft);
                    container->addHorizontalWrapMaxVariable(&mRight);
                } else {
                    int wrapStrength = SolverVariable::STRENGTH_EQUALITY;
                    system->addGreaterThan(system->createObjectVariable(&mParent->mRight), right, 0, wrapStrength);
                }
            }
        }
        if (mResolvedVertical) {
            system->addEquality(top, mY);
            system->addEquality(bottom, mY + mHeight);
            if (mBaseline.hasDependents()) {
                system->addEquality(baseline, mY + mBaselineDistance);
            }
            if (verticalParentWrapContent && mParent != nullptr) {
                if (mOptimizeWrapOnResolved) {
                    ConstraintWidgetContainer* container = dynamic_cast<ConstraintWidgetContainer*>(mParent);
                    container->addVerticalWrapMinVariable(&mTop);
                    container->addVerticalWrapMaxVariable(&mBottom);
                } else {
                    int wrapStrength = SolverVariable::STRENGTH_EQUALITY;
                    system->addGreaterThan(system->createObjectVariable(&mParent->mBottom), bottom, 0, wrapStrength);
                }
            }
        }
        if (mResolvedHorizontal && mResolvedVertical) {
            mResolvedHorizontal = false;
            mResolvedVertical = false;
            if constexpr (FULL_DEBUG) {
                std::cout << "\n----------------------------------------------\n";
                std::cout << "-- setting COMPLETED for " << getDebugName() << "\n";
                std::cout << "----------------------------------------------\n";
            }
            return;
        }
    }

    // Optimization check removed (analyzer)

    bool inHorizontalChain = false;
    bool inVerticalChain = false;

    if (mParent != nullptr) {
        if (isChainHead(HORIZONTAL)) {
            dynamic_cast<ConstraintWidgetContainer*>(mParent)->addChain(this, HORIZONTAL);
            inHorizontalChain = true;
        } else {
            inHorizontalChain = isInHorizontalChain();
        }

        if (isChainHead(VERTICAL)) {
            dynamic_cast<ConstraintWidgetContainer*>(mParent)->addChain(this, VERTICAL);
            inVerticalChain = true;
        } else {
            inVerticalChain = isInVerticalChain();
        }

        if (!inHorizontalChain && horizontalParentWrapContent && mVisibility != GONE && mLeft.mTarget == nullptr && mRight.mTarget == nullptr) {
            if constexpr (FULL_DEBUG) std::cout << "<>1 ADDING H WRAP GREATER FOR " << getDebugName() << std::endl;
            SolverVariable* parentRight = system->createObjectVariable(&mParent->mRight);
            system->addGreaterThan(parentRight, right, 0, SolverVariable::STRENGTH_LOW);
        }

        if (!inVerticalChain && verticalParentWrapContent && mVisibility != GONE && mTop.mTarget == nullptr && mBottom.mTarget == nullptr && !mHasBaseline) {
            if constexpr (FULL_DEBUG) std::cout << "<>1 ADDING V WRAP GREATER FOR " << getDebugName() << std::endl;
            SolverVariable* parentBottom = system->createObjectVariable(&mParent->mBottom);
            system->addGreaterThan(parentBottom, bottom, 0, SolverVariable::STRENGTH_LOW);
        }
    }

    int width = mWidth;
    if (width < mMinWidth) width = mMinWidth;
    int height = mHeight;
    if (height < mMinHeight) height = mMinHeight;

    bool horizontalDimensionFixed = mListDimensionBehaviors[DIMENSION_HORIZONTAL] != DimensionBehaviour::MATCH_CONSTRAINT;
    bool verticalDimensionFixed = mListDimensionBehaviors[DIMENSION_VERTICAL] != DimensionBehaviour::MATCH_CONSTRAINT;

    bool useRatio = false;
    mResolvedDimensionRatioSide = mDimensionRatioSide;
    mResolvedDimensionRatio = mDimensionRatio;

    int matchConstraintDefaultWidth = mMatchConstraintDefaultWidth;
    int matchConstraintDefaultHeight = mMatchConstraintDefaultHeight;

    if (mDimensionRatio > 0 && mVisibility != GONE) {
        useRatio = true;
        if (mListDimensionBehaviors[DIMENSION_HORIZONTAL] == DimensionBehaviour::MATCH_CONSTRAINT && matchConstraintDefaultWidth == MATCH_CONSTRAINT_SPREAD) {
            matchConstraintDefaultWidth = MATCH_CONSTRAINT_RATIO;
        }
        if (mListDimensionBehaviors[DIMENSION_VERTICAL] == DimensionBehaviour::MATCH_CONSTRAINT && matchConstraintDefaultHeight == MATCH_CONSTRAINT_SPREAD) {
            matchConstraintDefaultHeight = MATCH_CONSTRAINT_RATIO;
        }

        if (mListDimensionBehaviors[DIMENSION_HORIZONTAL] == DimensionBehaviour::MATCH_CONSTRAINT && mListDimensionBehaviors[DIMENSION_VERTICAL] == DimensionBehaviour::MATCH_CONSTRAINT && matchConstraintDefaultWidth == MATCH_CONSTRAINT_RATIO && matchConstraintDefaultHeight == MATCH_CONSTRAINT_RATIO) {
            setupDimensionRatio(horizontalParentWrapContent, verticalParentWrapContent, horizontalDimensionFixed, verticalDimensionFixed);
        } else if (mListDimensionBehaviors[DIMENSION_HORIZONTAL] == DimensionBehaviour::MATCH_CONSTRAINT && matchConstraintDefaultWidth == MATCH_CONSTRAINT_RATIO) {
            mResolvedDimensionRatioSide = HORIZONTAL;
            width = static_cast<int>(mResolvedDimensionRatio * mHeight);
            if (mListDimensionBehaviors[DIMENSION_VERTICAL] != DimensionBehaviour::MATCH_CONSTRAINT) {
                matchConstraintDefaultWidth = MATCH_CONSTRAINT_RATIO_RESOLVED;
                useRatio = false;
            }
        } else if (mListDimensionBehaviors[DIMENSION_VERTICAL] == DimensionBehaviour::MATCH_CONSTRAINT && matchConstraintDefaultHeight == MATCH_CONSTRAINT_RATIO) {
            mResolvedDimensionRatioSide = VERTICAL;
            if (mDimensionRatioSide == UNKNOWN) {
                mResolvedDimensionRatio = 1.0f / mResolvedDimensionRatio;
            }
            height = static_cast<int>(mResolvedDimensionRatio * mWidth);
            if (mListDimensionBehaviors[DIMENSION_HORIZONTAL] != DimensionBehaviour::MATCH_CONSTRAINT) {
                matchConstraintDefaultHeight = MATCH_CONSTRAINT_RATIO_RESOLVED;
                useRatio = false;
            }
        }
    }

    mResolvedMatchConstraintDefault[HORIZONTAL] = matchConstraintDefaultWidth;
    mResolvedMatchConstraintDefault[VERTICAL] = matchConstraintDefaultHeight;
    mResolvedHasRatio = useRatio;

    bool useHorizontalRatio = useRatio && (mResolvedDimensionRatioSide == HORIZONTAL || mResolvedDimensionRatioSide == UNKNOWN);
    bool useVerticalRatio = useRatio && (mResolvedDimensionRatioSide == VERTICAL || mResolvedDimensionRatioSide == UNKNOWN);

    bool wrapContent = (mListDimensionBehaviors[DIMENSION_HORIZONTAL] == DimensionBehaviour::WRAP_CONTENT) && (dynamic_cast<ConstraintWidgetContainer*>(this) != nullptr);
    if (wrapContent) {
        width = 0;
    }

    bool applyPosition = true;
    if (mCenter.isConnected()) {
        applyPosition = false;
    }

    bool isInHorizontalBarrier = mIsInBarrier[HORIZONTAL];
    bool isInVerticalBarrier = mIsInBarrier[VERTICAL];

    if (mHorizontalResolution != DIRECT && !mResolvedHorizontal) {
        SolverVariable* parentMax = mParent != nullptr ? system->createObjectVariable(&mParent->mRight) : nullptr;
        SolverVariable* parentMin = mParent != nullptr ? system->createObjectVariable(&mParent->mLeft) : nullptr;
        applyConstraints(system, true, horizontalParentWrapContent, verticalParentWrapContent, isTerminalWidget[HORIZONTAL], parentMin, parentMax, mListDimensionBehaviors[DIMENSION_HORIZONTAL], wrapContent, &mLeft, &mRight, mX, width, mMinWidth, mMaxDimension[HORIZONTAL], mHorizontalBiasPercent, useHorizontalRatio, mListDimensionBehaviors[VERTICAL] == DimensionBehaviour::MATCH_CONSTRAINT, inHorizontalChain, inVerticalChain, isInHorizontalBarrier, matchConstraintDefaultWidth, matchConstraintDefaultHeight, mMatchConstraintMinWidth, mMatchConstraintMaxWidth, mMatchConstraintPercentWidth, applyPosition);
    }
    bool applyVerticalConstraints = true;
    if (mVerticalResolution == DIRECT) {
        applyVerticalConstraints = false;
    }
    if (applyVerticalConstraints && !mResolvedVertical) {
        wrapContent = (mListDimensionBehaviors[DIMENSION_VERTICAL] == DimensionBehaviour::WRAP_CONTENT) && (dynamic_cast<ConstraintWidgetContainer*>(this) != nullptr);
        if (wrapContent) {
            height = 0;
        }

        SolverVariable* parentMax = mParent != nullptr ? system->createObjectVariable(&mParent->mBottom) : nullptr;
        SolverVariable* parentMin = mParent != nullptr ? system->createObjectVariable(&mParent->mTop) : nullptr;

        if (mBaselineDistance > 0 || mVisibility == GONE) {
            if (mBaseline.mTarget != nullptr) {
                system->addEquality(baseline, top, getBaselineDistance(), SolverVariable::STRENGTH_FIXED);
                SolverVariable* baselineTarget = system->createObjectVariable(mBaseline.mTarget);
                int baselineMargin = mBaseline.getMargin();
                system->addEquality(baseline, baselineTarget, baselineMargin, SolverVariable::STRENGTH_FIXED);
                applyPosition = false;
                if (verticalParentWrapContent) {
                    SolverVariable* end = system->createObjectVariable(&mBottom);
                    int wrapStrength = SolverVariable::STRENGTH_EQUALITY;
                    system->addGreaterThan(parentMax, end, 0, wrapStrength);
                }
            } else if (mVisibility == GONE) {
                system->addEquality(baseline, top, mBaseline.getMargin(), SolverVariable::STRENGTH_FIXED);
            } else {
                system->addEquality(baseline, top, getBaselineDistance(), SolverVariable::STRENGTH_FIXED);
            }
        }

        applyConstraints(system, false, verticalParentWrapContent, horizontalParentWrapContent, isTerminalWidget[VERTICAL], parentMin, parentMax, mListDimensionBehaviors[DIMENSION_VERTICAL], wrapContent, &mTop, &mBottom, mY, height, mMinHeight, mMaxDimension[VERTICAL], mVerticalBiasPercent, useVerticalRatio, mListDimensionBehaviors[HORIZONTAL] == DimensionBehaviour::MATCH_CONSTRAINT, inVerticalChain, inHorizontalChain, isInVerticalBarrier, matchConstraintDefaultHeight, matchConstraintDefaultWidth, mMatchConstraintMinHeight, mMatchConstraintMaxHeight, mMatchConstraintPercentHeight, applyPosition);
    }

    if (useRatio) {
        int strength = SolverVariable::STRENGTH_FIXED;
        if (mResolvedDimensionRatioSide == VERTICAL) {
            system->addRatio(bottom, top, right, left, mResolvedDimensionRatio, strength);
        } else {
            system->addRatio(right, left, bottom, top, mResolvedDimensionRatio, strength);
        }
    }

    if (mCenter.isConnected()) {
        system->addCenterPoint(this, mCenter.getTarget()->getOwner(), (float)/*toRadians*/ 0.017453292519943295 * (mCircleConstraintAngle + 90), mCenter.getMargin());
    }

    mResolvedHorizontal = false;
    mResolvedVertical = false;
}

// 2977: applyConstraints
void ConstraintWidget::applyConstraints(LinearSystem* system, bool isHorizontal, bool parentWrapContent, bool oppositeParentWrapContent, bool isTerminal, void* parentMinVoid, void* parentMaxVoid, DimensionBehaviour dimensionBehaviour, bool wrapContent, ConstraintAnchor* beginAnchor, ConstraintAnchor* endAnchor, int beginPosition, int dimension, int minDimension, int maxDimension, float bias, bool useRatio, bool oppositeVariable, bool inChain, bool oppositeInChain, bool inBarrier, int matchConstraintDefault, int oppositeMatchConstraintDefault, int matchMinDimension, int matchMaxDimension, float matchPercentDimension, bool applyPosition) {
    SolverVariable* parentMin = static_cast<SolverVariable*>(parentMinVoid);
    SolverVariable* parentMax = static_cast<SolverVariable*>(parentMaxVoid);
    
    SolverVariable* begin = system->createObjectVariable(beginAnchor);
    SolverVariable* end = system->createObjectVariable(endAnchor);
    SolverVariable* beginTarget = system->createObjectVariable(beginAnchor->getTarget());
    SolverVariable* endTarget = system->createObjectVariable(endAnchor->getTarget());

    bool isBeginConnected = beginAnchor->isConnected();
    bool isEndConnected = endAnchor->isConnected();
    bool isCenterConnected = mCenter.isConnected();

    bool variableSize = false;

    int numConnections = 0;
    if (isBeginConnected) numConnections++;
    if (isEndConnected) numConnections++;
    if (isCenterConnected) numConnections++;

    if (useRatio) {
        matchConstraintDefault = MATCH_CONSTRAINT_RATIO;
    }
    switch (dimensionBehaviour) {
        case DimensionBehaviour::FIXED: variableSize = false; break;
        case DimensionBehaviour::WRAP_CONTENT: variableSize = false; break;
        case DimensionBehaviour::MATCH_PARENT: variableSize = false; break;
        case DimensionBehaviour::MATCH_CONSTRAINT: variableSize = matchConstraintDefault != MATCH_CONSTRAINT_RATIO_RESOLVED; break;
    }

    if (mWidthOverride != -1 && isHorizontal) {
        variableSize = false;
        dimension = mWidthOverride;
        mWidthOverride = -1;
    }
    if (mHeightOverride != -1 && !isHorizontal) {
        variableSize = false;
        dimension = mHeightOverride;
        mHeightOverride = -1;
    }

    if (mVisibility == ConstraintWidget::GONE) {
        dimension = 0;
        variableSize = false;
    }

    if (applyPosition) {
        if (!isBeginConnected && !isEndConnected && !isCenterConnected) {
            system->addEquality(begin, beginPosition);
        } else if (isBeginConnected && !isEndConnected) {
            system->addEquality(begin, beginTarget, beginAnchor->getMargin(), SolverVariable::STRENGTH_FIXED);
        }
    }

    if (!variableSize) {
        if (wrapContent) {
            system->addEquality(end, begin, 0, SolverVariable::STRENGTH_HIGH);
            if (minDimension > 0) {
                system->addGreaterThan(end, begin, minDimension, SolverVariable::STRENGTH_FIXED);
            }
            if (maxDimension < 2147483647) {
                system->addLowerThan(end, begin, maxDimension, SolverVariable::STRENGTH_FIXED);
            }
        } else {
            system->addEquality(end, begin, dimension, SolverVariable::STRENGTH_FIXED);
        }
    } else {
        if (numConnections != 2 && !useRatio && (matchConstraintDefault == MATCH_CONSTRAINT_WRAP || matchConstraintDefault == MATCH_CONSTRAINT_SPREAD)) {
            variableSize = false;
            int d = std::max(matchMinDimension, dimension);
            if (matchMaxDimension > 0) {
                d = std::min(matchMaxDimension, d);
            }
            system->addEquality(end, begin, d, SolverVariable::STRENGTH_FIXED);
        } else {
            if (matchMinDimension == WRAP) matchMinDimension = dimension;
            if (matchMaxDimension == WRAP) matchMaxDimension = dimension;
            if (dimension > 0 && matchConstraintDefault != MATCH_CONSTRAINT_WRAP) {
                if (USE_WRAP_DIMENSION_FOR_SPREAD && (matchConstraintDefault == MATCH_CONSTRAINT_SPREAD)) {
                    system->addGreaterThan(end, begin, dimension, SolverVariable::STRENGTH_HIGHEST);
                }
                dimension = 0;
            }

            if (matchMinDimension > 0) {
                system->addGreaterThan(end, begin, matchMinDimension, SolverVariable::STRENGTH_FIXED);
                dimension = std::max(dimension, matchMinDimension);
            }
            if (matchMaxDimension > 0) {
                bool applyLimit = true;
                if (parentWrapContent && matchConstraintDefault == MATCH_CONSTRAINT_WRAP) {
                    applyLimit = false;
                }
                if (applyLimit) {
                    system->addLowerThan(end, begin, matchMaxDimension, SolverVariable::STRENGTH_FIXED);
                }
                dimension = std::min(dimension, matchMaxDimension);
            }
            if (matchConstraintDefault == MATCH_CONSTRAINT_WRAP) {
                if (parentWrapContent) {
                    system->addEquality(end, begin, dimension, SolverVariable::STRENGTH_FIXED);
                } else if (inChain) {
                    system->addEquality(end, begin, dimension, SolverVariable::STRENGTH_EQUALITY);
                    system->addLowerThan(end, begin, dimension, SolverVariable::STRENGTH_FIXED);
                } else {
                    system->addEquality(end, begin, dimension, SolverVariable::STRENGTH_EQUALITY);
                    system->addLowerThan(end, begin, dimension, SolverVariable::STRENGTH_FIXED);
                }
            } else if (matchConstraintDefault == MATCH_CONSTRAINT_PERCENT) {
                SolverVariable* percentBegin = nullptr;
                SolverVariable* percentEnd = nullptr;
                if (beginAnchor->getType() == ConstraintAnchor::Type::TOP || beginAnchor->getType() == ConstraintAnchor::Type::BOTTOM) {
                    percentBegin = system->createObjectVariable(mParent->getAnchor(static_cast<int>(ConstraintAnchor::Type::TOP)));
                    percentEnd = system->createObjectVariable(mParent->getAnchor(static_cast<int>(ConstraintAnchor::Type::BOTTOM)));
                } else {
                    percentBegin = system->createObjectVariable(mParent->getAnchor(static_cast<int>(ConstraintAnchor::Type::LEFT)));
                    percentEnd = system->createObjectVariable(mParent->getAnchor(static_cast<int>(ConstraintAnchor::Type::RIGHT)));
                }
                if (parentWrapContent) {
                    variableSize = false;
                }
            } else {
                isTerminal = true;
            }
        }
    }

    if (!applyPosition || inChain) {
        if (numConnections < 2 && parentWrapContent && isTerminal) {
            system->addGreaterThan(begin, parentMin, 0, SolverVariable::STRENGTH_FIXED);
            bool applyEnd = isHorizontal || (mBaseline.mTarget == nullptr);
            if (!isHorizontal && mBaseline.mTarget != nullptr) {
                ConstraintWidget* target = mBaseline.mTarget->mOwner;
                if (target->mDimensionRatio != 0 && target->mListDimensionBehaviors[0] == DimensionBehaviour::MATCH_CONSTRAINT && target->mListDimensionBehaviors[1] == DimensionBehaviour::MATCH_CONSTRAINT) {
                    applyEnd = true;
                } else {
                    applyEnd = false;
                }
            }
            if (applyEnd) {
                system->addGreaterThan(parentMax, end, 0, SolverVariable::STRENGTH_FIXED);
            }
        }
        return;
    }

    int wrapStrength = SolverVariable::STRENGTH_EQUALITY;

    if (!isBeginConnected && !isEndConnected && !isCenterConnected) {
    } else if (isBeginConnected && !isEndConnected) {
        ConstraintWidget* beginWidget = beginAnchor->mTarget->mOwner;
        if (parentWrapContent && dynamic_cast<Barrier*>(beginWidget) != nullptr) {
            wrapStrength = SolverVariable::STRENGTH_FIXED;
        }
    } else if (!isBeginConnected && isEndConnected) {
        system->addEquality(end, endTarget, -endAnchor->getMargin(), SolverVariable::STRENGTH_FIXED);
        if (parentWrapContent) {
            if (mOptimizeWrapO && begin && begin->isFinalValue && mParent != nullptr) {
                ConstraintWidgetContainer* container = dynamic_cast<ConstraintWidgetContainer*>(mParent);
                if (isHorizontal) {
                    container->addHorizontalWrapMinVariable(beginAnchor);
                } else {
                    container->addVerticalWrapMinVariable(beginAnchor);
                }
            } else {
                system->addGreaterThan(begin, parentMin, 0, SolverVariable::STRENGTH_EQUALITY);
            }
        }
    } else if (isBeginConnected && isEndConnected) {
        bool applyBoundsCheck = true;
        bool applyCentering = false;
        bool applyStrongChecks = false;
        bool applyRangeCheck = false;
        int rangeCheckStrength = SolverVariable::STRENGTH_EQUALITY;
        int boundsCheckStrength = SolverVariable::STRENGTH_HIGHEST;
        int centeringStrength = SolverVariable::STRENGTH_BARRIER;

        if (parentWrapContent) {
            rangeCheckStrength = SolverVariable::STRENGTH_EQUALITY;
        }
        ConstraintWidget* beginWidget = beginAnchor->mTarget->mOwner;
        ConstraintWidget* endWidget = endAnchor->mTarget->mOwner;
        ConstraintWidget* parent = getParent();

        if (variableSize) {
            if (matchConstraintDefault == MATCH_CONSTRAINT_SPREAD) {
                if (matchMaxDimension == 0 && matchMinDimension == 0) {
                    applyStrongChecks = true;
                    rangeCheckStrength = SolverVariable::STRENGTH_FIXED;
                    boundsCheckStrength = SolverVariable::STRENGTH_FIXED;
                    if (beginTarget && beginTarget->isFinalValue && endTarget && endTarget->isFinalValue) {
                        system->addEquality(begin, beginTarget, beginAnchor->getMargin(), SolverVariable::STRENGTH_FIXED);
                        system->addEquality(end, endTarget, -endAnchor->getMargin(), SolverVariable::STRENGTH_FIXED);
                        return;
                    }
                } else {
                    applyCentering = true;
                    rangeCheckStrength = SolverVariable::STRENGTH_EQUALITY;
                    boundsCheckStrength = SolverVariable::STRENGTH_EQUALITY;
                    applyBoundsCheck = true;
                    applyRangeCheck = true;
                }
                if (dynamic_cast<Barrier*>(beginWidget) != nullptr || dynamic_cast<Barrier*>(endWidget) != nullptr) {
                    boundsCheckStrength = SolverVariable::STRENGTH_HIGHEST;
                }
            } else if (matchConstraintDefault == MATCH_CONSTRAINT_PERCENT) {
                applyCentering = true;
                rangeCheckStrength = SolverVariable::STRENGTH_EQUALITY;
                boundsCheckStrength = SolverVariable::STRENGTH_EQUALITY;
                applyBoundsCheck = true;
                applyRangeCheck = true;
                if (dynamic_cast<Barrier*>(beginWidget) != nullptr || dynamic_cast<Barrier*>(endWidget) != nullptr) {
                    boundsCheckStrength = SolverVariable::STRENGTH_HIGHEST;
                }
            } else if (matchConstraintDefault == MATCH_CONSTRAINT_WRAP) {
                applyCentering = true;
                applyRangeCheck = true;
                rangeCheckStrength = SolverVariable::STRENGTH_FIXED;
            } else if (matchConstraintDefault == MATCH_CONSTRAINT_RATIO) {
                if (mResolvedDimensionRatioSide == UNKNOWN) {
                    applyCentering = true;
                    applyRangeCheck = true;
                    applyStrongChecks = true;
                    rangeCheckStrength = SolverVariable::STRENGTH_FIXED;
                    boundsCheckStrength = SolverVariable::STRENGTH_EQUALITY;
                    if (oppositeInChain) {
                        boundsCheckStrength = SolverVariable::STRENGTH_EQUALITY;
                        centeringStrength = SolverVariable::STRENGTH_HIGHEST;
                        if (parentWrapContent) {
                            centeringStrength = SolverVariable::STRENGTH_EQUALITY;
                        }
                    } else {
                        centeringStrength = SolverVariable::STRENGTH_FIXED;
                    }
                } else {
                    applyCentering = true;
                    applyRangeCheck = true;
                    applyStrongChecks = true;
                    if (useRatio) {
                        bool otherSideInvariable = oppositeMatchConstraintDefault == MATCH_CONSTRAINT_PERCENT || oppositeMatchConstraintDefault == MATCH_CONSTRAINT_WRAP;
                        if (!otherSideInvariable) {
                            rangeCheckStrength = SolverVariable::STRENGTH_FIXED;
                            boundsCheckStrength = SolverVariable::STRENGTH_EQUALITY;
                        }
                    } else {
                        rangeCheckStrength = SolverVariable::STRENGTH_EQUALITY;
                        if (matchMaxDimension > 0) {
                            boundsCheckStrength = SolverVariable::STRENGTH_EQUALITY;
                        } else if (matchMaxDimension == 0 && matchMinDimension == 0) {
                            if (!oppositeInChain) {
                                boundsCheckStrength = SolverVariable::STRENGTH_FIXED;
                            } else {
                                if (beginWidget != parent && endWidget != parent) {
                                    rangeCheckStrength = SolverVariable::STRENGTH_HIGHEST;
                                } else {
                                    rangeCheckStrength = SolverVariable::STRENGTH_EQUALITY;
                                }
                                boundsCheckStrength = SolverVariable::STRENGTH_HIGHEST;
                            }
                        }
                    }
                }
            }
        } else {
            applyCentering = true;
            applyRangeCheck = true;
            if (beginTarget && beginTarget->isFinalValue && endTarget && endTarget->isFinalValue) {
                system->addCentering(begin, beginTarget, beginAnchor->getMargin(), bias, endTarget, end, endAnchor->getMargin(), SolverVariable::STRENGTH_FIXED);
                if (parentWrapContent && isTerminal) {
                    int margin = 0;
                    if (endAnchor->mTarget != nullptr) margin = endAnchor->getMargin();
                    if (endTarget != parentMax) {
                        system->addGreaterThan(parentMax, end, margin, wrapStrength);
                    }
                }
                return;
            }
        }

        if (applyRangeCheck && beginTarget == endTarget && beginWidget != parent) {
            applyRangeCheck = false;
            applyBoundsCheck = false;
        }

        if (applyCentering) {
            if (!variableSize && !oppositeVariable && !oppositeInChain && beginTarget == parentMin && endTarget == parentMax) {
                centeringStrength = SolverVariable::STRENGTH_FIXED;
                rangeCheckStrength = SolverVariable::STRENGTH_FIXED;
                applyBoundsCheck = false;
                parentWrapContent = false;
            }
            system->addCentering(begin, beginTarget, beginAnchor->getMargin(), bias, endTarget, end, endAnchor->getMargin(), centeringStrength);
        }

        if (mVisibility == GONE && !endAnchor->hasDependents()) {
            return;
        }

        if (applyRangeCheck) {
            if (parentWrapContent && beginTarget != endTarget && !variableSize) {
                if (dynamic_cast<Barrier*>(beginWidget) != nullptr || dynamic_cast<Barrier*>(endWidget) != nullptr) {
                    rangeCheckStrength = SolverVariable::STRENGTH_BARRIER;
                }
            }
            system->addGreaterThan(begin, beginTarget, beginAnchor->getMargin(), rangeCheckStrength);
            system->addLowerThan(end, endTarget, -endAnchor->getMargin(), rangeCheckStrength);
        }

        if (parentWrapContent && inBarrier && !(dynamic_cast<Barrier*>(beginWidget) != nullptr || dynamic_cast<Barrier*>(endWidget) != nullptr) && !(endWidget == parent)) {
            boundsCheckStrength = SolverVariable::STRENGTH_BARRIER;
            rangeCheckStrength = SolverVariable::STRENGTH_BARRIER;
            applyBoundsCheck = true;
        }

        if (applyBoundsCheck) {
            if (applyStrongChecks && (!oppositeInChain || oppositeParentWrapContent)) {
                int strength = boundsCheckStrength;
                if (beginWidget == parent || endWidget == parent) strength = SolverVariable::STRENGTH_BARRIER;
                if (dynamic_cast<Guideline*>(beginWidget) != nullptr || dynamic_cast<Guideline*>(endWidget) != nullptr) strength = SolverVariable::STRENGTH_EQUALITY;
                if (dynamic_cast<Barrier*>(beginWidget) != nullptr || dynamic_cast<Barrier*>(endWidget) != nullptr) strength = SolverVariable::STRENGTH_EQUALITY;
                if (oppositeInChain) strength = SolverVariable::STRENGTH_EQUALITY;
                boundsCheckStrength = std::max(strength, boundsCheckStrength);
            }

            if (parentWrapContent) {
                boundsCheckStrength = std::min(rangeCheckStrength, boundsCheckStrength);
                if (useRatio && !oppositeInChain && (beginWidget == parent || endWidget == parent)) {
                    boundsCheckStrength = SolverVariable::STRENGTH_HIGHEST;
                }
            }
            system->addEquality(begin, beginTarget, beginAnchor->getMargin(), boundsCheckStrength);
            system->addEquality(end, endTarget, -endAnchor->getMargin(), boundsCheckStrength);
        }

        if (parentWrapContent) {
            int margin = 0;
            if (parentMin == beginTarget) margin = beginAnchor->getMargin();
            if (beginTarget != parentMin) {
                system->addGreaterThan(begin, parentMin, margin, wrapStrength);
            }
        }

        if (parentWrapContent && variableSize && minDimension == 0 && matchMinDimension == 0) {
            if (variableSize && matchConstraintDefault == MATCH_CONSTRAINT_RATIO) {
                system->addGreaterThan(end, begin, 0, SolverVariable::STRENGTH_FIXED);
            } else {
                system->addGreaterThan(end, begin, 0, wrapStrength);
            }
        }
    }

    if (parentWrapContent && isTerminal) {
        int margin = 0;
        if (endAnchor->mTarget != nullptr) margin = endAnchor->getMargin();
        if (endTarget != parentMax) {
            if (mOptimizeWrapO && end && end->isFinalValue && mParent != nullptr) {
                ConstraintWidgetContainer* container = dynamic_cast<ConstraintWidgetContainer*>(mParent);
                if (isHorizontal) {
                    container->addHorizontalWrapMaxVariable(endAnchor);
                } else {
                    container->addVerticalWrapMaxVariable(endAnchor);
                }
                return;
            }
            system->addGreaterThan(parentMax, end, margin, wrapStrength);
        }
    }
}

} // namespace setu::cassowary
