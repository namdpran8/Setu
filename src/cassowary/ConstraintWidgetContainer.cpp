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

#include "ConstraintWidgetContainer.h"
#include "ConstraintWidget.h"
#include "Optimizer.h"
#include "Tier2Stubs.h"
#include "ConstraintAnchor.h"
#include "LinearSystem.h"
#include "Chain.h"
#include <iostream>
#include <algorithm>
#include <cmath>
#include <cassert>

namespace setu::cassowary {

// 144: public void fillMetrics(Metrics metrics)
void ConstraintWidgetContainer::fillMetrics(Metrics* metrics) {
    mMetrics = metrics;
    mSystem.fillMetrics(metrics);
}

// 182: public ConstraintWidgetContainer()
ConstraintWidgetContainer::ConstraintWidgetContainer() {
}

// 193: public ConstraintWidgetContainer(int x, int y, int width, int height)
ConstraintWidgetContainer::ConstraintWidgetContainer(int x, int y, int width, int height)
    : WidgetContainer(x, y, width, height) {
}

// 203: public ConstraintWidgetContainer(int width, int height)
ConstraintWidgetContainer::ConstraintWidgetContainer(int width, int height)
    : WidgetContainer(width, height) {
}

// 207: public ConstraintWidgetContainer(String debugName, int width, int height)
ConstraintWidgetContainer::ConstraintWidgetContainer(const std::string& debugName, int width, int height)
    : WidgetContainer(width, height) {
    setDebugName(debugName);
}

ConstraintWidgetContainer::~ConstraintWidgetContainer() {
    resetChains();
}

// 217: public void setOptimizationLevel(int value)
void ConstraintWidgetContainer::setOptimizationLevel(int value) {
    mOptimizationLevel = value;
    LinearSystem::USE_DEPENDENCY_ORDERING = optimizeFor(Optimizer::OPTIMIZATION_DEPENDENCY_ORDERING);
}

// 225: public int getOptimizationLevel()
int ConstraintWidgetContainer::getOptimizationLevel() const {
    return mOptimizationLevel;
}

// 232: public boolean optimizeFor(int feature)
bool ConstraintWidgetContainer::optimizeFor(int feature) const {
    return (mOptimizationLevel & feature) == feature;
}

// 239: public String getType()
std::string ConstraintWidgetContainer::getType() const {
    return "ConstraintLayout";
}

// 244: public void reset()
void ConstraintWidgetContainer::reset() {
    mSystem.reset();
    mPaddingLeft = 0;
    mPaddingRight = 0;
    mPaddingTop = 0;
    mPaddingBottom = 0;
    mSkipSolver = false;
    mVerticalWrapMin = nullptr;
    mHorizontalWrapMin = nullptr;
    mVerticalWrapMax = nullptr;
    mHorizontalWrapMax = nullptr;
    WidgetContainer::reset();
}

// 258: public boolean isWidthMeasuredTooSmall()
bool ConstraintWidgetContainer::isWidthMeasuredTooSmall() const {
    return mWidthMeasuredTooSmall;
}

// 265: public boolean isHeightMeasuredTooSmall()
bool ConstraintWidgetContainer::isHeightMeasuredTooSmall() const {
    return mHeightMeasuredTooSmall;
}

// 276: void addVerticalWrapMinVariable(ConstraintAnchor top)
void ConstraintWidgetContainer::addVerticalWrapMinVariable(ConstraintAnchor* top) {
    if (mVerticalWrapMin == nullptr || top->getFinalValue() > mVerticalWrapMin->getFinalValue()) {
        mVerticalWrapMin = top;
    }
}

// 284: public void addHorizontalWrapMinVariable(ConstraintAnchor left)
void ConstraintWidgetContainer::addHorizontalWrapMinVariable(ConstraintAnchor* left) {
    if (mHorizontalWrapMin == nullptr || left->getFinalValue() > mHorizontalWrapMin->getFinalValue()) {
        mHorizontalWrapMin = left;
    }
}

// 291: void addVerticalWrapMaxVariable(ConstraintAnchor bottom)
void ConstraintWidgetContainer::addVerticalWrapMaxVariable(ConstraintAnchor* bottom) {
    if (mVerticalWrapMax == nullptr || bottom->getFinalValue() > mVerticalWrapMax->getFinalValue()) {
        mVerticalWrapMax = bottom;
    }
}

// 299: public void addHorizontalWrapMaxVariable(ConstraintAnchor right)
void ConstraintWidgetContainer::addHorizontalWrapMaxVariable(ConstraintAnchor* right) {
    if (mHorizontalWrapMax == nullptr || right->getFinalValue() > mHorizontalWrapMax->getFinalValue()) {
        mHorizontalWrapMax = right;
    }
}

// 306: private void addMinWrap(ConstraintAnchor constraintAnchor, SolverVariable parentMin)
void ConstraintWidgetContainer::addMinWrap(ConstraintAnchor* constraintAnchor, SolverVariable* parentMin) {
    SolverVariable* variable = mSystem.createObjectVariable(constraintAnchor);
    int wrapStrength = SolverVariable::STRENGTH_EQUALITY;
    mSystem.addGreaterThan(variable, parentMin, 0, wrapStrength);
}

// 312: private void addMaxWrap(ConstraintAnchor constraintAnchor, SolverVariable parentMax)
void ConstraintWidgetContainer::addMaxWrap(ConstraintAnchor* constraintAnchor, SolverVariable* parentMax) {
    SolverVariable* variable = mSystem.createObjectVariable(constraintAnchor);
    int wrapStrength = SolverVariable::STRENGTH_EQUALITY;
    mSystem.addGreaterThan(parentMax, variable, 0, wrapStrength);
}

// 325: public boolean addChildrenToSolver(LinearSystem system)
bool ConstraintWidgetContainer::addChildrenToSolver(LinearSystem& system) {
    if constexpr (DEBUG) {
        std::cout << "\n#######################################\n";
        std::cout << "##    ADD CHILDREN TO SOLVER  (" << mDebugSolverPassCount << ") ##\n";
        std::cout << "#######################################\n\n";
        mDebugSolverPassCount++;
    }

    bool optimize = optimizeFor(Optimizer::OPTIMIZATION_GRAPH);
    addToSolver(&system, optimize);
    const int count = mChildren.size();

    bool hasBarriers = false;
    for (int i = 0; i < count; i++) {
        ConstraintWidget* widget = mChildren[i].get();
        widget->setInBarrier(HORIZONTAL, false);
        widget->setInBarrier(VERTICAL, false);
        if (dynamic_cast<Barrier*>(widget) != nullptr) {
            hasBarriers = true;
        }
    }

    if (hasBarriers) {
        for (int i = 0; i < count; i++) {
            ConstraintWidget* widget = mChildren[i].get();
            if (auto barrier = dynamic_cast<Barrier*>(widget)) {
                barrier->markWidgets();
            }
        }
    }

    mWidgetsToAdd.clear();
    for (int i = 0; i < count; i++) {
        ConstraintWidget* widget = mChildren[i].get();
        if (widget->addFirst()) {
            if (dynamic_cast<VirtualLayout*>(widget) != nullptr) {
                mWidgetsToAdd.insert(widget);
            } else {
                widget->addToSolver(&system, optimize);
            }
        }
    }

    // If we have virtual layouts, we need to add them to the solver in the correct
    // order (in case they reference one another)
    while (!mWidgetsToAdd.empty()) {
        int numLayouts = mWidgetsToAdd.size();
        VirtualLayout* layout = nullptr;
        for (auto it = mWidgetsToAdd.begin(); it != mWidgetsToAdd.end(); ++it) {
            layout = dynamic_cast<VirtualLayout*>(*it);
            if (layout && layout->contains(mWidgetsToAdd)) {
                layout->addToSolver(&system, optimize);
                mWidgetsToAdd.erase(it);
                break;
            }
        }
        if (numLayouts == mWidgetsToAdd.size()) {
            // looks we didn't find anymore dependency, let's add everything.
            for (ConstraintWidget* widget : mWidgetsToAdd) {
                widget->addToSolver(&system, optimize);
            }
            mWidgetsToAdd.clear();
        }
    }

    if (LinearSystem::USE_DEPENDENCY_ORDERING) {
        std::set<ConstraintWidget*> widgetsToAdd;
        for (int i = 0; i < count; i++) {
            ConstraintWidget* widget = mChildren[i].get();
            if (!widget->addFirst()) {
                widgetsToAdd.insert(widget);
            }
        }
        int orientation = VERTICAL;
        if (getHorizontalDimensionBehaviour() == DimensionBehaviour::WRAP_CONTENT) {
            orientation = HORIZONTAL;
        }
        // Assuming addChildrenToSolverByDependency is a static method in Optimizer or similar helper
        Optimizer::addChildrenToSolverByDependency(this, &system, widgetsToAdd, orientation, false);
        for (ConstraintWidget* widget : widgetsToAdd) {
            Optimizer::checkMatchParent(this, &system, widget);
            widget->addToSolver(&system, optimize);
        }
    } else {
        for (int i = 0; i < count; i++) {
            ConstraintWidget* widget = mChildren[i].get();
            if (dynamic_cast<ConstraintWidgetContainer*>(widget) != nullptr) {
                DimensionBehaviour horizontalBehaviour = widget->mListDimensionBehaviors[DIMENSION_HORIZONTAL];
                DimensionBehaviour verticalBehaviour = widget->mListDimensionBehaviors[DIMENSION_VERTICAL];
                if (horizontalBehaviour == DimensionBehaviour::WRAP_CONTENT) {
                    widget->setHorizontalDimensionBehaviour(DimensionBehaviour::FIXED);
                }
                if (verticalBehaviour == DimensionBehaviour::WRAP_CONTENT) {
                    widget->setVerticalDimensionBehaviour(DimensionBehaviour::FIXED);
                }
                widget->addToSolver(&system, optimize);
                if (horizontalBehaviour == DimensionBehaviour::WRAP_CONTENT) {
                    widget->setHorizontalDimensionBehaviour(horizontalBehaviour);
                }
                if (verticalBehaviour == DimensionBehaviour::WRAP_CONTENT) {
                    widget->setVerticalDimensionBehaviour(verticalBehaviour);
                }
            } else {
                Optimizer::checkMatchParent(this, &system, widget);
                if (!widget->addFirst()) {
                    widget->addToSolver(&system, optimize);
                }
            }
        }
    }

    // Chains
    if (mHorizontalChainsSize > 0) {
        Chain::applyChainConstraints(this, &system, mHorizontalChainsArray, mHorizontalChainsSize, HORIZONTAL);
    }
    if (mVerticalChainsSize > 0) {
        Chain::applyChainConstraints(this, &system, mVerticalChainsArray, mVerticalChainsSize, VERTICAL);
    }
    return true;
}

// 455: public boolean updateChildrenFromSolver(LinearSystem system, boolean[] flags)
bool ConstraintWidgetContainer::updateChildrenFromSolver(LinearSystem& system, std::vector<bool>& flags) {
    flags[Optimizer::FLAG_RECOMPUTE_BOUNDS] = false;
    bool optimize = optimizeFor(Optimizer::OPTIMIZATION_GRAPH);
    updateFromSolver(&system, optimize);
    const int count = mChildren.size();
    bool hasOverride = false;
    for (int i = 0; i < count; i++) {
        ConstraintWidget* widget = mChildren[i].get();
        widget->updateFromSolver(&system, optimize);
        if (widget->hasDimensionOverride()) {
            hasOverride = true;
        }
    }
    return hasOverride;
}

// 472: public void updateFromRuns(boolean updateHorizontal, boolean updateVertical)
void ConstraintWidgetContainer::updateFromRuns(bool updateHorizontal, bool updateVertical) {
    WidgetContainer::updateFromRuns(updateHorizontal, updateVertical);
    const int count = mChildren.size();
    for (int i = 0; i < count; i++) {
        ConstraintWidget* widget = mChildren[i].get();
        widget->updateFromRuns(updateHorizontal, updateVertical);
    }
}

// 489: public void setPadding(int left, int top, int right, int bottom)
void ConstraintWidgetContainer::setPadding(int left, int top, int right, int bottom) {
    mPaddingLeft = left;
    mPaddingTop = top;
    mPaddingRight = right;
    mPaddingBottom = bottom;
}

// 501: public void setRtl(boolean isRtl)
void ConstraintWidgetContainer::setRtl(bool isRtl) {
    mIsRtl = isRtl;
}

// 510: public boolean isRtl()
bool ConstraintWidgetContainer::isRtl() const {
    return mIsRtl;
}

// 667: public void layout()
void ConstraintWidgetContainer::layout() {
    if constexpr (DEBUG) {
        std::cout << "\n#####################################\n";
        std::cout << "##          CL LAYOUT PASS           ##\n";
        std::cout << "#####################################\n\n";
        mDebugSolverPassCount = 0;
    }

    mX = 0;
    mY = 0;

    mWidthMeasuredTooSmall = false;
    mHeightMeasuredTooSmall = false;
    const int count = mChildren.size();

    int preW = std::max(0, getWidth());
    int preH = std::max(0, getHeight());
    DimensionBehaviour originalVerticalDimensionBehaviour = mListDimensionBehaviors[DIMENSION_VERTICAL];
    DimensionBehaviour originalHorizontalDimensionBehaviour = mListDimensionBehaviors[DIMENSION_HORIZONTAL];

    if constexpr (DEBUG_LAYOUT) {
        std::cout << "layout with preW: " << preW << " ("
                  << (int)mListDimensionBehaviors[DIMENSION_HORIZONTAL] << ") preH: " << preH
                  << " (" << (int)mListDimensionBehaviors[DIMENSION_VERTICAL] << ")\n";
    }

    if (mMetrics != nullptr) {
        mMetrics->layouts++;
    }

    bool wrap_override = false;

    if constexpr (DEBUG) {
        std::cout << "OPTIMIZATION LEVEL " << mOptimizationLevel << "\n";
    }

    bool useGraphOptimizer = optimizeFor(Optimizer::OPTIMIZATION_GRAPH)
            || optimizeFor(Optimizer::OPTIMIZATION_GRAPH_WRAP);

    mSystem.graphOptimizer = false;
    mSystem.newgraphOptimizer = false;

    if (mOptimizationLevel != Optimizer::OPTIMIZATION_NONE && useGraphOptimizer) {
        mSystem.newgraphOptimizer = true;
    }

    int countSolve = 0;

    bool hasWrapContent = getHorizontalDimensionBehaviour() == DimensionBehaviour::WRAP_CONTENT
            || getVerticalDimensionBehaviour() == DimensionBehaviour::WRAP_CONTENT;

    // Reset the chains before iterating on our children
    resetChains();
    countSolve = 0;

    // Before we solve our system, we should call layout() on any
    // of our children that is a container.
    for (int i = 0; i < count; i++) {
        ConstraintWidget* widget = mChildren[i].get();
        if (auto container = dynamic_cast<WidgetContainer*>(widget)) {
            container->layout();
        }
    }
    bool optimize = optimizeFor(Optimizer::OPTIMIZATION_GRAPH);

    // Now let's solve our system as usual
    bool needsSolving = true;
    while (needsSolving) {
        countSolve++;
        try {
            mSystem.reset();
            resetChains();
            if constexpr (DEBUG) {
                std::string debugName = getDebugName();
                if (debugName.empty()) {
                    debugName = "root";
                }
                setDebugSolverName(&mSystem, debugName);
                for (int i = 0; i < count; i++) {
                    ConstraintWidget* widget = mChildren[i].get();
                    if (!widget->getDebugName().empty()) {
                        widget->setDebugSolverName(&mSystem, widget->getDebugName());
                    }
                }
            } else {
                createObjectVariables(&mSystem);
                for (int i = 0; i < count; i++) {
                    ConstraintWidget* widget = mChildren[i].get();
                    widget->createObjectVariables(&mSystem);
                }
            }
            needsSolving = addChildrenToSolver(mSystem);
            
            if (mVerticalWrapMin != nullptr) {
                addMinWrap(mVerticalWrapMin, mSystem.createObjectVariable(&mTop));
                mVerticalWrapMin = nullptr;
            }
            if (mVerticalWrapMax != nullptr) {
                addMaxWrap(mVerticalWrapMax, mSystem.createObjectVariable(&mBottom));
                mVerticalWrapMax = nullptr;
            }
            if (mHorizontalWrapMin != nullptr) {
                addMinWrap(mHorizontalWrapMin, mSystem.createObjectVariable(&mLeft));
                mHorizontalWrapMin = nullptr;
            }
            if (mHorizontalWrapMax != nullptr) {
                addMaxWrap(mHorizontalWrapMax, mSystem.createObjectVariable(&mRight));
                mHorizontalWrapMax = nullptr;
            }
            
            if (needsSolving) {
                mSystem.minimize();
            }
        } catch (const std::exception& e) {
            std::cout << "EXCEPTION : " << e.what() << "\n";
        }
        
        if (needsSolving) {
            needsSolving = updateChildrenFromSolver(mSystem, Optimizer::sFlags);
        } else {
            updateFromSolver(&mSystem, optimize);
            for (int i = 0; i < count; i++) {
                ConstraintWidget* widget = mChildren[i].get();
                widget->updateFromSolver(&mSystem, optimize);
            }
            needsSolving = false;
        }

        if (hasWrapContent && countSolve < MAX_ITERATIONS
                && Optimizer::sFlags[Optimizer::FLAG_RECOMPUTE_BOUNDS]) {
            // let's get the new bounds
            int maxX = 0;
            int maxY = 0;
            for (int i = 0; i < count; i++) {
                ConstraintWidget* widget = mChildren[i].get();
                maxX = std::max(maxX, widget->getX() + widget->getWidth());
                maxY = std::max(maxY, widget->getY() + widget->getHeight());
            }
            maxX = std::max(mMinWidth, maxX);
            maxY = std::max(mMinHeight, maxY);
            
            if (originalHorizontalDimensionBehaviour == DimensionBehaviour::WRAP_CONTENT) {
                if (getWidth() < maxX) {
                    if constexpr (DEBUG_LAYOUT) {
                        std::cout << countSolve << "layout override width from " << getWidth() << " vs " << maxX << "\n";
                    }
                    setWidth(maxX);
                    // force using the solver
                    mListDimensionBehaviors[DIMENSION_HORIZONTAL] = DimensionBehaviour::WRAP_CONTENT;
                    wrap_override = true;
                    needsSolving = true;
                }
            }
            if (originalVerticalDimensionBehaviour == DimensionBehaviour::WRAP_CONTENT) {
                if (getHeight() < maxY) {
                    if constexpr (DEBUG_LAYOUT) {
                        std::cout << "layout override height from " << getHeight() << " vs " << maxY << "\n";
                    }
                    setHeight(maxY);
                    // force using the solver
                    mListDimensionBehaviors[DIMENSION_VERTICAL] = DimensionBehaviour::WRAP_CONTENT;
                    wrap_override = true;
                    needsSolving = true;
                }
            }
        }
        
        if (true) {
            int width = std::max(mMinWidth, getWidth());
            if (width > getWidth()) {
                if constexpr (DEBUG_LAYOUT) {
                    std::cout << "layout override 2, width from " << getWidth() << " vs " << width << "\n";
                }
                setWidth(width);
                mListDimensionBehaviors[DIMENSION_HORIZONTAL] = DimensionBehaviour::FIXED;
                wrap_override = true;
                needsSolving = true;
            }
            int height = std::max(mMinHeight, getHeight());
            if (height > getHeight()) {
                if constexpr (DEBUG_LAYOUT) {
                    std::cout << "layout override 2, height from " << getHeight() << " vs " << height << "\n";
                }
                setHeight(height);
                mListDimensionBehaviors[DIMENSION_VERTICAL] = DimensionBehaviour::FIXED;
                wrap_override = true;
                needsSolving = true;
            }

            if (!wrap_override) {
                if (mListDimensionBehaviors[DIMENSION_HORIZONTAL] == DimensionBehaviour::WRAP_CONTENT
                        && preW > 0) {
                    if (getWidth() > preW) {
                        if constexpr (DEBUG_LAYOUT) {
                            std::cout << "layout override 3, width from " << getWidth() << " vs " << preW << "\n";
                        }
                        mWidthMeasuredTooSmall = true;
                        wrap_override = true;
                        mListDimensionBehaviors[DIMENSION_HORIZONTAL] = DimensionBehaviour::FIXED;
                        setWidth(preW);
                        needsSolving = true;
                    }
                }
                if (mListDimensionBehaviors[DIMENSION_VERTICAL] == DimensionBehaviour::WRAP_CONTENT
                        && preH > 0) {
                    if (getHeight() > preH) {
                        if constexpr (DEBUG_LAYOUT) {
                            std::cout << "layout override 3, height from " << getHeight() << " vs " << preH << "\n";
                        }
                        mHeightMeasuredTooSmall = true;
                        wrap_override = true;
                        mListDimensionBehaviors[DIMENSION_VERTICAL] = DimensionBehaviour::FIXED;
                        setHeight(preH);
                        needsSolving = true;
                    }
                }
            }

            if (countSolve > MAX_ITERATIONS) {
                needsSolving = false;
            }
        }
    }
    
    if constexpr (DEBUG_LAYOUT) {
        std::cout << "Solved system in " << countSolve << " iterations (" << getWidth() << " x " << getHeight() << ")\n";
    }



    if (wrap_override) {
        mListDimensionBehaviors[DIMENSION_HORIZONTAL] = originalHorizontalDimensionBehaviour;
        mListDimensionBehaviors[DIMENSION_VERTICAL] = originalVerticalDimensionBehaviour;
    }

    resetSolverVariables(mSystem.getCache());
}

// 1014: public boolean handlesInternalConstraints()
bool ConstraintWidgetContainer::handlesInternalConstraints() const {
    return false;
}

// 1027: public ArrayList<Guideline> getVerticalGuidelines()
std::vector<Guideline*> ConstraintWidgetContainer::getVerticalGuidelines() {
    std::vector<Guideline*> guidelines;
    for (auto& _w : mChildren) { ConstraintWidget* widget = _w.get();
        if (auto guideline = dynamic_cast<Guideline*>(widget)) {
            if (guideline->getOrientation() == Guideline::VERTICAL) {
                guidelines.push_back(guideline);
            }
        }
    }
    return guidelines;
}

// 1046: public ArrayList<Guideline> getHorizontalGuidelines()
std::vector<Guideline*> ConstraintWidgetContainer::getHorizontalGuidelines() {
    std::vector<Guideline*> guidelines;
    for (auto& _w : mChildren) { ConstraintWidget* widget = _w.get();
        if (auto guideline = dynamic_cast<Guideline*>(widget)) {
            if (guideline->getOrientation() == Guideline::HORIZONTAL) {
                guidelines.push_back(guideline);
            }
        }
    }
    return guidelines;
}

// 1060: public LinearSystem getSystem()
LinearSystem& ConstraintWidgetContainer::getSystem() {
    return mSystem;
}

// 1071: private void resetChains()
void ConstraintWidgetContainer::resetChains() {
    for (int i = 0; i < mHorizontalChainsSize; i++) {
        delete mHorizontalChainsArray[i];
        mHorizontalChainsArray[i] = nullptr;
    }
    for (int i = 0; i < mVerticalChainsSize; i++) {
        delete mVerticalChainsArray[i];
        mVerticalChainsArray[i] = nullptr;
    }
    mHorizontalChainsSize = 0;
    mVerticalChainsSize = 0;
}

// 1081: void addChain(ConstraintWidget constraintWidget, int type)
void ConstraintWidgetContainer::addChain(ConstraintWidget* constraintWidget, int type) {
    if (type == HORIZONTAL) {
        addHorizontalChain(constraintWidget);
    } else if (type == VERTICAL) {
        addVerticalChain(constraintWidget);
    }
}

// 1096: private void addHorizontalChain(ConstraintWidget widget)
void ConstraintWidgetContainer::addHorizontalChain(ConstraintWidget* widget) {
    if (mHorizontalChainsSize + 1 >= (int)mHorizontalChainsArray.size()) {
        mHorizontalChainsArray.resize(mHorizontalChainsArray.size() == 0 ? 4 : mHorizontalChainsArray.size() * 2, nullptr);
    }
    mHorizontalChainsArray[mHorizontalChainsSize] = new ChainHead(widget, HORIZONTAL, isRtl());
    mHorizontalChainsSize++;
}

// 1111: private void addVerticalChain(ConstraintWidget widget)
void ConstraintWidgetContainer::addVerticalChain(ConstraintWidget* widget) {
    if (mVerticalChainsSize + 1 >= (int)mVerticalChainsArray.size()) {
        mVerticalChainsArray.resize(mVerticalChainsArray.size() == 0 ? 4 : mVerticalChainsArray.size() * 2, nullptr);
    }
    mVerticalChainsArray[mVerticalChainsSize] = new ChainHead(widget, VERTICAL, isRtl());
    mVerticalChainsSize++;
}

// 1123: public void setPass(int pass)
void ConstraintWidgetContainer::setPass(int pass) {
    this->mPass = pass;
}

// 1129: public void getSceneString(StringBuilder ret)
void ConstraintWidgetContainer::getSceneString(std::string& ret) {
    ret += stringId + ":{\n";
    ret += "  actualWidth:" + std::to_string(mWidth) + "\n";
    ret += "  actualHeight:" + std::to_string(mHeight) + "\n";

    for (auto& w : mChildren) {
        ConstraintWidget* child = w.get();
        child->getSceneString(ret);
        ret += ",\n";
    }
    ret += "}";
}

} // namespace setu::cassowary
