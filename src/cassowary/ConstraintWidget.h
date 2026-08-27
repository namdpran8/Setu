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

#include <string>
#include <vector>
#include <array>
#include <unordered_map>
#include <unordered_set>
#include <cmath>
#include <cassert>

#include "ConstraintAnchor.h"

namespace setu::cassowary {

class ConstraintAnchor;
class WidgetContainer;
class ConstraintWidgetContainer;
class LinearSystem;
class Cache;

class ConstraintWidget {
public:
    static constexpr bool DEBUG = false;
    static constexpr bool FULL_DEBUG = false;
    static constexpr bool AUTOTAG_CENTER = false;
    static constexpr bool DO_NOT_USE = false;
    static constexpr int SOLVER = 1;
    static constexpr int DIRECT = 2;
    static constexpr bool USE_WRAP_DIMENSION_FOR_SPREAD = false;

    bool measured = false;

    std::array<bool, 2> isTerminalWidget = {true, true};
    bool mResolvedHasRatio = false;
private:
    bool mMeasureRequested = true;
    bool mOptimizeWrapO = false;
    bool mOptimizeWrapOnResolved = true;

    int mWidthOverride = -1;
    int mHeightOverride = -1;

public:
    std::string stringId;

private:
    bool mResolvedHorizontal = false;
    bool mResolvedVertical = false;

    bool mHorizontalSolvingPass = false;
    bool mVerticalSolvingPass = false;

public:
    void setFinalFrame(int left, int top, int right, int bottom, int baseline, int orientation);
    void setFinalLeft(int x1);
    void setFinalTop(int y1);
    void resetSolvingPassFlag();
    bool isHorizontalSolvingPassDone() const;
    bool isVerticalSolvingPassDone() const;
    void markHorizontalSolvingPassDone();
    void markVerticalSolvingPassDone();
    void setFinalHorizontal(int x1, int x2);
    void setFinalVertical(int y1, int y2);
    void setFinalBaseline(int baselineValue);
    bool isResolvedHorizontally() const;
    bool isResolvedVertically() const;
    void resetFinalResolution();
    void ensureMeasureRequested();
    bool hasDependencies() const;
    bool hasDanglingDimension(int orientation) const;
    bool hasResolvedTargets(int orientation, int size) const;

    static constexpr int MATCH_CONSTRAINT_SPREAD = 0;
    static constexpr int MATCH_CONSTRAINT_WRAP = 1;
    static constexpr int MATCH_CONSTRAINT_PERCENT = 2;
    static constexpr int MATCH_CONSTRAINT_RATIO = 3;
    static constexpr int MATCH_CONSTRAINT_RATIO_RESOLVED = 4;

    static constexpr int UNKNOWN = -1;
    static constexpr int HORIZONTAL = 0;
    static constexpr int VERTICAL = 1;
    static constexpr int BOTH = 2;

    static constexpr int VISIBLE = 0;
    static constexpr int INVISIBLE = 4;
    static constexpr int GONE = 8;

    static constexpr int CHAIN_SPREAD = 0;
    static constexpr int CHAIN_SPREAD_INSIDE = 1;
    static constexpr int CHAIN_PACKED = 2;

    static constexpr int WRAP_BEHAVIOR_INCLUDED = 0;
    static constexpr int WRAP_BEHAVIOR_HORIZONTAL_ONLY = 1;
    static constexpr int WRAP_BEHAVIOR_VERTICAL_ONLY = 2;
    static constexpr int WRAP_BEHAVIOR_SKIPPED = 3;

    int mHorizontalResolution = UNKNOWN;
    int mVerticalResolution = UNKNOWN;

private:
    static constexpr int WRAP = -2;
    int mWrapBehaviorInParent = WRAP_BEHAVIOR_INCLUDED;

public:
    int mMatchConstraintDefaultWidth = MATCH_CONSTRAINT_SPREAD;
    int mMatchConstraintDefaultHeight = MATCH_CONSTRAINT_SPREAD;
    std::array<int, 2> mResolvedMatchConstraintDefault = {0, 0};

    int mMatchConstraintMinWidth = 0;
    int mMatchConstraintMaxWidth = 0;
    float mMatchConstraintPercentWidth = 1.0f;
    int mMatchConstraintMinHeight = 0;
    int mMatchConstraintMaxHeight = 0;
    float mMatchConstraintPercentHeight = 1.0f;
    bool mIsWidthWrapContent = false;
    bool mIsHeightWrapContent = false;

    int mResolvedDimensionRatioSide = UNKNOWN;
    float mResolvedDimensionRatio = 1.0f;

private:
    std::array<int, 2> mMaxDimension = {2147483647, 2147483647}; // Integer.MAX_VALUE
public:
    float mCircleConstraintAngle = NAN;
private:
    bool mHasBaseline = false;
    bool mInPlaceholder = false;
    bool mInVirtualLayout = false;

public:
    bool isInVirtualLayout() const;
    void setInVirtualLayout(bool inVirtualLayout);
    int getMaxHeight() const;
    int getMaxWidth() const;
    void setMaxWidth(int maxWidth);
    void setMaxHeight(int maxHeight);
    bool isSpreadWidth() const;
    bool isSpreadHeight() const;
    void setHasBaseline(bool hasBaseline);
    bool getHasBaseline() const;
    bool isInPlaceholder() const;
    void setInPlaceholder(bool inPlaceholder);

public:
    void setInBarrier(int orientation, bool value);

public:
    bool isInBarrier(int orientation) const;
    void setMeasureRequested(bool measureRequested);
    bool isMeasureRequested() const;
    void setWrapBehaviorInParent(int behavior);
    int getWrapBehaviorInParent() const;

private:
    int mLastHorizontalMeasureSpec = 0;
    int mLastVerticalMeasureSpec = 0;

public:
    int getLastHorizontalMeasureSpec() const;
    int getLastVerticalMeasureSpec() const;
    void setLastMeasureSpec(int horizontal, int vertical);

    enum class DimensionBehaviour {
        FIXED, WRAP_CONTENT, MATCH_CONSTRAINT, MATCH_PARENT
    };

    ConstraintAnchor mLeft;
    ConstraintAnchor mTop;
    ConstraintAnchor mRight;
    ConstraintAnchor mBottom;
    ConstraintAnchor mBaseline;
    ConstraintAnchor mCenterX;
    ConstraintAnchor mCenterY;
    ConstraintAnchor mCenter;

    static constexpr int ANCHOR_LEFT = 0;
    static constexpr int ANCHOR_RIGHT = 1;
    static constexpr int ANCHOR_TOP = 2;
    static constexpr int ANCHOR_BOTTOM = 3;
    static constexpr int ANCHOR_BASELINE = 4;

    std::array<ConstraintAnchor*, 6> mListAnchors;

public:
    std::vector<ConstraintAnchor*> mAnchors;

private:
    std::array<bool, 2> mIsInBarrier = {false, false};

public:
    static constexpr int DIMENSION_HORIZONTAL = 0;
    static constexpr int DIMENSION_VERTICAL = 1;
    std::array<DimensionBehaviour, 2> mListDimensionBehaviors = {DimensionBehaviour::FIXED, DimensionBehaviour::FIXED};

    ConstraintWidget* mParent = nullptr;

    int mWidth = 0;
    int mHeight = 0;
    float mDimensionRatio = 0;

public:
    int mDimensionRatioSide = UNKNOWN;
    int mX = 0;
    int mY = 0;

public:
    int mRelX = 0;
    int mRelY = 0;

public:
    int mOffsetX = 0;
    int mOffsetY = 0;

public:
    int mBaselineDistance = 0;

public:
    int mMinWidth = 0;
    int mMinHeight = 0;

public:
    static constexpr float DEFAULT_BIAS = 0.5f;
    float mHorizontalBiasPercent = DEFAULT_BIAS;
    float mVerticalBiasPercent = DEFAULT_BIAS;

private:
    void* mCompanionWidget = nullptr;
    int mContainerItemSkip = 0;
    int mVisibility = VISIBLE;
    bool mAnimated = false;
    std::string mDebugName;
    std::string mType;

public:
    int mDistToTop = 0;
    int mDistToLeft = 0;
    int mDistToRight = 0;
    int mDistToBottom = 0;
    bool mLeftHasCentered = false;
    bool mRightHasCentered = false;
    bool mTopHasCentered = false;
    bool mBottomHasCentered = false;
    bool mHorizontalWrapVisited = false;
    bool mVerticalWrapVisited = false;
    bool mGroupsToSolver = false;

    int mHorizontalChainStyle = CHAIN_SPREAD;
    int mVerticalChainStyle = CHAIN_SPREAD;
    bool mHorizontalChainFixedPosition = false;
    bool mVerticalChainFixedPosition = false;

    std::array<float, 2> mWeight = {static_cast<float>(UNKNOWN), static_cast<float>(UNKNOWN)};

public:
    std::array<ConstraintWidget*, 2> mListNextMatchConstraintsWidget = {nullptr, nullptr};
    std::array<ConstraintWidget*, 2> mNextChainWidget = {nullptr, nullptr};

public:
    ConstraintWidget* mHorizontalNextWidget = nullptr;
    ConstraintWidget* mVerticalNextWidget = nullptr;

    ConstraintWidget();
    ConstraintWidget(const std::string& debugName);
    ConstraintWidget(int x, int y, int width, int height);
    ConstraintWidget(const std::string& debugName, int x, int y, int width, int height);
    ConstraintWidget(int width, int height);
    ConstraintWidget(const std::string& debugName, int width, int height);

    virtual ~ConstraintWidget() = default;

    virtual void reset();

private:
    void serializeAnchor(std::string& ret, const std::string& side, ConstraintAnchor* a);
    void serializeCircle(std::string& ret, ConstraintAnchor* a, float angle);
    void serializeAttribute(std::string& ret, const std::string& type, float value, float def);
    void serializeAttribute(std::string& ret, const std::string& type, int value, int def);
    void serializeAttribute(std::string& ret, const std::string& type, const std::string& value, const std::string& def);
    void serializeDimensionRatio(std::string& ret, const std::string& type, float value, int whichSide);
    void serializeSize(std::string& ret, const std::string& type, int size, int min, int max, int override, int matchConstraintMin, int matchConstraintDefault, float matchConstraintPercent, float weight);

public:
    std::string serialize(std::string& ret);

    int horizontalGroup = -1;
    int verticalGroup = -1;

    bool oppositeDimensionDependsOn(int orientation) const;
    bool oppositeDimensionsTied() const;
    bool hasDimensionOverride() const;
    virtual void resetSolverVariables(Cache* cache);

private:
    void addAnchors();

public:
    bool isRoot() const;
    ConstraintWidget* getParent() const;
    void setParent(ConstraintWidget* widget);
    void setWidthWrapContent(bool widthWrapContent);
    bool isWidthWrapContent() const;
    void setHeightWrapContent(bool heightWrapContent);
    bool isHeightWrapContent() const;
    void connectCircularConstraint(ConstraintWidget* target, float angle, int radius);
    virtual std::string getType() const;
    void setType(const std::string& type);
    void setVisibility(int visibility);
    int getVisibility() const;
    void setAnimated(bool animated);
    bool isAnimated() const;
    std::string getDebugName() const;
    void setDebugName(const std::string& name);
    void setDebugSolverName(LinearSystem* system, const std::string& name);
    void createObjectVariables(LinearSystem* system);
    std::string toString() const;
    int getX() const;
    int getY() const;
    int getWidth() const;
    int getOptimizerWrapWidth() const;
    int getOptimizerWrapHeight() const;
    int getHeight() const;
    int getLength(int orientation) const;

public:
    int getRootX() const;
    int getRootY() const;

public:
    int getMinWidth() const;
    int getMinHeight() const;
    int getLeft() const;
    int getTop() const;
    int getRight() const;
    int getBottom() const;
    int getHorizontalMargin() const;
    int getVerticalMargin() const;
    float getHorizontalBiasPercent() const;
    float getVerticalBiasPercent() const;
    float getBiasPercent(int orientation) const;
    bool hasBaseline() const;
    int getBaselineDistance() const;
    void* getCompanionWidget() const;
    std::vector<ConstraintAnchor*>& getAnchors();
    void setX(int x);
    void setY(int y);
    void setOrigin(int x, int y);
    virtual void setOffset(int x, int y);
    void setOffset(int x, int y, int width, int height);
    // Note: Type corresponds to ConstraintAnchor::Type but C++ requires passing it properly.
    void setGoneMargin(int type, int goneMargin);
    void setWidth(int w);
    void setHeight(int h);
    void setLength(int length, int orientation);
    void setHorizontalMatchStyle(int horizontalMatchStyle, int min, int max, float percent);
    void setVerticalMatchStyle(int verticalMatchStyle, int min, int max, float percent);
    void setDimensionRatio(const std::string& ratio);
    void setDimensionRatio(float ratio, int dimensionRatioSide);
    float getDimensionRatio() const;
    int getDimensionRatioSide() const;
    void setHorizontalBiasPercent(float horizontalBiasPercent);
    void setVerticalBiasPercent(float verticalBiasPercent);
    void setMinWidth(int w);
    void setMinHeight(int h);
    void setDimension(int w, int h);
    void setFrame(int left, int top, int right, int bottom);
    void setFrame(int start, int end, int orientation);
    void setHorizontalDimension(int left, int right);
    void setVerticalDimension(int top, int bottom);
    int getRelativePositioning(int orientation) const;
    void setRelativePositioning(int offset, int orientation);
    void setBaselineDistance(int baseline);
    void setCompanionWidget(void* companion);
    void setContainerItemSkip(int skip);
    int getContainerItemSkip() const;
    void setHorizontalWeight(float horizontalWeight);
    void setVerticalWeight(float verticalWeight);
    void setHorizontalChainStyle(int horizontalChainStyle);
    int getHorizontalChainStyle() const;
    void setVerticalChainStyle(int verticalChainStyle);
    int getVerticalChainStyle() const;
public:
    virtual bool allowedInBarrier() const;

    void immediateConnect(int startType, ConstraintWidget* target, int endType, int margin, int goneMargin);
    void connect(ConstraintAnchor* from, ConstraintAnchor* to, int margin);
    void connect(int constraintFrom, ConstraintWidget* target, int constraintTo);
    void connect(int constraintFrom, ConstraintWidget* target, int constraintTo, int margin);

    void resetAllConstraints();
    void resetAnchor(ConstraintAnchor* anchor);
    void resetAnchors();
    virtual ConstraintAnchor* getAnchor(int anchorType);
    virtual ConstraintAnchor* getAnchor(ConstraintAnchor::Type anchorType) {
        return getAnchor(static_cast<int>(anchorType));
    }

    DimensionBehaviour getHorizontalDimensionBehaviour() const;
    DimensionBehaviour getVerticalDimensionBehaviour() const;
    DimensionBehaviour getDimensionBehaviour(int orientation) const;
    void setHorizontalDimensionBehaviour(DimensionBehaviour behaviour);
    void setVerticalDimensionBehaviour(DimensionBehaviour behaviour);

    bool isInHorizontalChain() const;
    ConstraintWidget* getPreviousChainMember(int orientation) const;
    ConstraintWidget* getNextChainMember(int orientation) const;
    ConstraintWidget* getHorizontalChainControlWidget();
    bool isInVerticalChain() const;
    ConstraintWidget* getVerticalChainControlWidget();

private:
    bool isChainHead(int orientation) const;

public:
    virtual void addToSolver(LinearSystem* system, bool optimize);
    bool addFirst() const;
    void setupDimensionRatio(bool hParentWrapContent, bool vParentWrapContent, bool horizontalDimensionFixed, bool verticalDimensionFixed);

private:
    void applyConstraints(LinearSystem* system, bool isHorizontal, bool parentWrapContent, bool oppositeParentWrapContent, bool isTerminal, void* parentMin, void* parentMax, DimensionBehaviour dimensionBehaviour, bool wrapContent, ConstraintAnchor* beginAnchor, ConstraintAnchor* endAnchor, int beginPosition, int dimension, int minDimension, int maxDimension, float bias, bool useRatio, bool oppositeVariable, bool inChain, bool oppositeInChain, bool inBarrier, int matchConstraintDefault, int oppositeMatchConstraintDefault, int matchMinDimension, int matchMaxDimension, float matchPercentDimension, bool applyPosition);

public:
    virtual void updateFromSolver(LinearSystem* system, bool optimize);
    void copy(ConstraintWidget* src, std::unordered_map<ConstraintWidget*, ConstraintWidget*>& map);
    virtual void updateFromRuns(bool updateHorizontal, bool updateVertical);
    void addChildrenToSolverByDependency(ConstraintWidgetContainer* container, LinearSystem* system, std::unordered_set<ConstraintWidget*>& widgets, int orientation, bool addSelf);
    virtual void getSceneString(std::string& ret);

private:
    void getSceneString(std::string& ret, const std::string& type, int size, int min, int max, int override, int matchConstraintMin, int matchConstraintDefault, float matchConstraintPercent, DimensionBehaviour behavior, float weight);
    void getSceneString(std::string& ret, const std::string& side, ConstraintAnchor* a);
};

// Inline constructor implementations to initialize mListAnchors and mAnchors properly
inline ConstraintWidget::ConstraintWidget()
    : mLeft(this, ConstraintAnchor::Type::LEFT),
      mTop(this, ConstraintAnchor::Type::TOP),
      mRight(this, ConstraintAnchor::Type::RIGHT),
      mBottom(this, ConstraintAnchor::Type::BOTTOM),
      mBaseline(this, ConstraintAnchor::Type::BASELINE),
      mCenterX(this, ConstraintAnchor::Type::CENTER_X),
      mCenterY(this, ConstraintAnchor::Type::CENTER_Y),
      mCenter(this, ConstraintAnchor::Type::CENTER),
      mListAnchors({&mLeft, &mRight, &mTop, &mBottom, &mBaseline, &mCenter}) {
    addAnchors();
}

inline ConstraintWidget::ConstraintWidget(const std::string& debugName) : ConstraintWidget() {
    setDebugName(debugName);
}

inline ConstraintWidget::ConstraintWidget(int x, int y, int width, int height) : ConstraintWidget() {
    mX = x;
    mY = y;
    mWidth = width;
    mHeight = height;
}

inline ConstraintWidget::ConstraintWidget(const std::string& debugName, int x, int y, int width, int height) : ConstraintWidget(x, y, width, height) {
    setDebugName(debugName);
}

inline ConstraintWidget::ConstraintWidget(int width, int height) : ConstraintWidget(0, 0, width, height) {}

inline ConstraintWidget::ConstraintWidget(const std::string& debugName, int width, int height) : ConstraintWidget(width, height) {
    setDebugName(debugName);
}

} // namespace setu::cassowary
