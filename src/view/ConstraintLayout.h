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
#include "ViewGroup.h"
#include <unordered_map>
#include <string>
#include <vector>
#include <memory>

#include "../cassowary/ConstraintWidgetContainer.h"
#include "../cassowary/ConstraintWidget.h"

namespace setu {
namespace view {

class ConstraintLayout : public ViewGroup {
public:
    class LayoutParams : public View::LayoutParams {
    public:
        // Relationships to other view IDs or parent ("parent" typically mapped to 0)
        int topToTop = -1;
        int topToBottom = -1;
        int bottomToTop = -1;
        int bottomToBottom = -1;
        int startToStart = -1;
        int startToEnd = -1;
        int endToStart = -1;
        int endToEnd = -1;

        // Biases
        float horizontalBias = 0.5f;
        float verticalBias = 0.5f;

        // Guidelines
        bool isGuideline = false;
        int guideBegin = -1;
        int guideEnd = -1;
        float guidePercent = -1.0f;
        int orientation = 0; // 0 = horizontal, 1 = vertical

        LayoutParams(int w, int h) : View::LayoutParams(w, h) {}
    };

    ConstraintLayout();
    virtual ~ConstraintLayout() = default;

    void onMeasure(int widthMeasureSpec, int heightMeasureSpec) override;
    void onLayout(bool changed, int l, int t, int r, int b) override;

    std::shared_ptr<View::LayoutParams> generateLayoutParams(android::ResXMLParser* parser) override;

    // Phase 1: manual registration
    void addConstrainedChild(std::shared_ptr<View> child);

    void addView(std::shared_ptr<View> child) override;
    void onFinishInflate() override;
    
    // For testing
    cassowary::ConstraintWidget* getWidget(View* child) const {
        auto it = mViewToWidget.find(child);
        return it != mViewToWidget.end() ? it->second : nullptr;
    }


private:
    cassowary::ConstraintWidgetContainer mLayoutWidget;
    std::unordered_map<View*, cassowary::ConstraintWidget*> mViewToWidget;
    std::unordered_map<cassowary::ConstraintWidget*, View*> mWidgetToView;
};

} // namespace view
} // namespace setu