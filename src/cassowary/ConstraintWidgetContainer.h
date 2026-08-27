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

#include "WidgetContainer.h"
#include "LinearSystem.h"
#include "Metrics.h"
#include <vector>
#include <string>
#include <set>

namespace setu::cassowary {

class ChainHead;
class Guideline;
class ConstraintAnchor;
class SolverVariable;

class ConstraintWidgetContainer : public WidgetContainer {
public:
    static constexpr int MAX_ITERATIONS = 8;

    static constexpr bool DEBUG = false;
    static constexpr bool DEBUG_LAYOUT = false;
    static constexpr bool DEBUG_GRAPH = false;

private:
    int mPass = 0; // number of layout passes

public:
    Metrics* mMetrics = nullptr;

    // 144: public void fillMetrics(Metrics metrics)
    void fillMetrics(Metrics* metrics);

protected:
    LinearSystem mSystem;

public:
    int mPaddingLeft = 0;
    int mPaddingTop = 0;
    int mPaddingRight = 0;
    int mPaddingBottom = 0;

    int mHorizontalChainsSize = 0;
    int mVerticalChainsSize = 0;

    std::vector<ChainHead*> mVerticalChainsArray;
    std::vector<ChainHead*> mHorizontalChainsArray;

    bool mGroupsWrapOptimized = false;
    bool mHorizontalWrapOptimized = false;
    bool mVerticalWrapOptimized = false;
    int mWrapFixedWidth = 0;
    int mWrapFixedHeight = 0;

private:
    int mOptimizationLevel = 257; // Optimizer::OPTIMIZATION_STANDARD
public:
    bool mSkipSolver = false;

private:
    bool mWidthMeasuredTooSmall = false;
    bool mHeightMeasuredTooSmall = false;

    int mDebugSolverPassCount = 0;

    ConstraintAnchor* mVerticalWrapMin = nullptr;
    ConstraintAnchor* mHorizontalWrapMin = nullptr;
    ConstraintAnchor* mVerticalWrapMax = nullptr;
    ConstraintAnchor* mHorizontalWrapMax = nullptr;

    std::set<ConstraintWidget*> mWidgetsToAdd;
    
    bool mIsRtl = false;

public:
    // 182: public ConstraintWidgetContainer()
    ConstraintWidgetContainer();

    // 193: public ConstraintWidgetContainer(int x, int y, int width, int height)
    ConstraintWidgetContainer(int x, int y, int width, int height);

    // 203: public ConstraintWidgetContainer(int width, int height)
    ConstraintWidgetContainer(int width, int height);

    // 207: public ConstraintWidgetContainer(String debugName, int width, int height)
    ConstraintWidgetContainer(const std::string& debugName, int width, int height);

    ~ConstraintWidgetContainer() override;

    // 217: public void setOptimizationLevel(int value)
    void setOptimizationLevel(int value);

    // 225: public int getOptimizationLevel()
    int getOptimizationLevel() const;

    // 232: public boolean optimizeFor(int feature)
    bool optimizeFor(int feature) const;

    // 239: public String getType()
    std::string getType() const override;

    // 244: public void reset()
    void reset() override;

    // 258: public boolean isWidthMeasuredTooSmall()
    bool isWidthMeasuredTooSmall() const;

    // 265: public boolean isHeightMeasuredTooSmall()
    bool isHeightMeasuredTooSmall() const;

    // 276: void addVerticalWrapMinVariable(ConstraintAnchor top)
    void addVerticalWrapMinVariable(ConstraintAnchor* top);

    // 284: public void addHorizontalWrapMinVariable(ConstraintAnchor left)
    void addHorizontalWrapMinVariable(ConstraintAnchor* left);

    // 291: void addVerticalWrapMaxVariable(ConstraintAnchor bottom)
    void addVerticalWrapMaxVariable(ConstraintAnchor* bottom);

    // 299: public void addHorizontalWrapMaxVariable(ConstraintAnchor right)
    void addHorizontalWrapMaxVariable(ConstraintAnchor* right);

private:
    // 306: private void addMinWrap(ConstraintAnchor constraintAnchor, SolverVariable parentMin)
    void addMinWrap(ConstraintAnchor* constraintAnchor, SolverVariable* parentMin);

    // 312: private void addMaxWrap(ConstraintAnchor constraintAnchor, SolverVariable parentMax)
    void addMaxWrap(ConstraintAnchor* constraintAnchor, SolverVariable* parentMax);

public:
    // 325: public boolean addChildrenToSolver(LinearSystem system)
    bool addChildrenToSolver(LinearSystem& system);

    // 455: public boolean updateChildrenFromSolver(LinearSystem system, boolean[] flags)
    bool updateChildrenFromSolver(LinearSystem& system, std::vector<bool>& flags);

    // 472: public void updateFromRuns(boolean updateHorizontal, boolean updateVertical)
    void updateFromRuns(bool updateHorizontal, bool updateVertical) override;

    // 489: public void setPadding(int left, int top, int right, int bottom)
    void setPadding(int left, int top, int right, int bottom);

    // 501: public void setRtl(boolean isRtl)
    void setRtl(bool isRtl);

    // 510: public boolean isRtl()
    bool isRtl() const;

    // 667: public void layout()
    void layout() override;

    // 1014: public boolean handlesInternalConstraints()
    bool handlesInternalConstraints() const;

    // 1027: public ArrayList<Guideline> getVerticalGuidelines()
    std::vector<Guideline*> getVerticalGuidelines();

    // 1046: public ArrayList<Guideline> getHorizontalGuidelines()
    std::vector<Guideline*> getHorizontalGuidelines();

    // 1060: public LinearSystem getSystem()
    LinearSystem& getSystem();

private:
    // 1071: private void resetChains()
    void resetChains();

    // 1096: private void addHorizontalChain(ConstraintWidget widget)
    void addHorizontalChain(ConstraintWidget* widget);

    // 1111: private void addVerticalChain(ConstraintWidget widget)
    void addVerticalChain(ConstraintWidget* widget);

public:
    // 1081: void addChain(ConstraintWidget constraintWidget, int type)
    void addChain(ConstraintWidget* constraintWidget, int type);

    // 1123: public void setPass(int pass)
    void setPass(int pass);

    // 1129: public void getSceneString(StringBuilder ret)
    void getSceneString(std::string& ret) override;
};

} // namespace setu::cassowary
