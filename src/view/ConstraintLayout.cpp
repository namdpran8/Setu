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

#include "androidfw/Util.h"
#include "ConstraintLayout.h"
#include <algorithm>
#include "../utils/Logger.h"
#include "androidfw/ResourceTypes.h"
#include "../cassowary/Guideline.h"

namespace setu {
namespace view {

ConstraintLayout::ConstraintLayout() {
    mLayoutWidget.setCompanionWidget(this);
}

void ConstraintLayout::addConstrainedChild(std::shared_ptr<View> child) {
    addView(child); // wait, this would loop. Let's just keep addConstrainedChild for testing and have addView call the logic.
}

void ConstraintLayout::addView(std::shared_ptr<View> child) {
    ViewGroup::addView(child);
    
    cassowary::ConstraintWidget* widgetPtr = nullptr;
    auto lp = std::dynamic_pointer_cast<LayoutParams>(child->getLayoutParams());
    
    if (lp && lp->isGuideline) {
        auto guideline = std::make_unique<cassowary::Guideline>();
        guideline->setOrientation(lp->orientation);
        if (lp->guidePercent >= 0) guideline->setGuidePercent(lp->guidePercent);
        else if (lp->guideBegin >= 0) guideline->setGuideBegin(lp->guideBegin);
        else if (lp->guideEnd >= 0) guideline->setGuideEnd(lp->guideEnd);
        
        widgetPtr = guideline.get();
        widgetPtr->setCompanionWidget(child.get());
        mLayoutWidget.add(std::move(guideline));
    } else {
        auto widget = std::make_unique<cassowary::ConstraintWidget>();
        widgetPtr = widget.get();
        widgetPtr->setCompanionWidget(child.get());
        mLayoutWidget.add(std::move(widget));
    }
    
    mViewToWidget[child.get()] = widgetPtr;
    mWidgetToView[widgetPtr] = child.get();
}


void ConstraintLayout::onFinishInflate() {
    // Pass 2: Apply constraints
    for (auto& child : mChildren) {
        cassowary::ConstraintWidget* widget = mViewToWidget[child.get()];
        if (!widget) continue;
        
        auto lp = std::dynamic_pointer_cast<LayoutParams>(child->getLayoutParams());
        if (!lp) continue;

        if (lp->isGuideline) continue; // guidelines already configured in addView

        auto resolveTarget = [&](int targetId) -> cassowary::ConstraintWidget* {
            if (targetId == 0) return &mLayoutWidget;
            auto targetView = findViewById(targetId);
            if (targetView) return mViewToWidget[targetView.get()];
            return nullptr;
        };

        if (lp->topToTop != -1) {
            if (auto t = resolveTarget(lp->topToTop)) widget->mTop.connect(t->getAnchor(cassowary::ConstraintAnchor::Type::TOP), lp->topMargin);
        }
        if (lp->topToBottom != -1) {
            if (auto t = resolveTarget(lp->topToBottom)) widget->mTop.connect(t->getAnchor(cassowary::ConstraintAnchor::Type::BOTTOM), lp->topMargin);
        }
        if (lp->bottomToTop != -1) {
            if (auto t = resolveTarget(lp->bottomToTop)) widget->mBottom.connect(t->getAnchor(cassowary::ConstraintAnchor::Type::TOP), lp->bottomMargin);
        }
        if (lp->bottomToBottom != -1) {
            if (auto t = resolveTarget(lp->bottomToBottom)) widget->mBottom.connect(t->getAnchor(cassowary::ConstraintAnchor::Type::BOTTOM), lp->bottomMargin);
        }
        if (lp->startToStart != -1) {
            if (auto t = resolveTarget(lp->startToStart)) widget->mLeft.connect(t->getAnchor(cassowary::ConstraintAnchor::Type::LEFT), lp->leftMargin);
        }
        if (lp->startToEnd != -1) {
            if (auto t = resolveTarget(lp->startToEnd)) widget->mLeft.connect(t->getAnchor(cassowary::ConstraintAnchor::Type::RIGHT), lp->leftMargin);
        }
        if (lp->endToStart != -1) {
            if (auto t = resolveTarget(lp->endToStart)) widget->mRight.connect(t->getAnchor(cassowary::ConstraintAnchor::Type::LEFT), lp->rightMargin);
        }
        if (lp->endToEnd != -1) {
            if (auto t = resolveTarget(lp->endToEnd)) widget->mRight.connect(t->getAnchor(cassowary::ConstraintAnchor::Type::RIGHT), lp->rightMargin);
        }
        
        widget->setHorizontalBiasPercent(lp->horizontalBias);
        widget->setVerticalBiasPercent(lp->verticalBias);
    }
}

void ConstraintLayout::onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
    int widthMode = getMode(widthMeasureSpec);
    int widthSize = getSize(widthMeasureSpec);
    int heightMode = getMode(heightMeasureSpec);
    int heightSize = getSize(heightMeasureSpec);

    // Sync layout widget bounds
    mLayoutWidget.setWidth(widthMode == MEASURE_SPEC_EXACTLY ? widthSize : 0);
    mLayoutWidget.setHeight(heightMode == MEASURE_SPEC_EXACTLY ? heightSize : 0);
    
    // Pre-measure children to get WRAP_CONTENT sizes
    for (auto& child : mChildren) {
        if (child->getVisibility() == View::GONE) {
            continue;
        }

        auto lp = std::dynamic_pointer_cast<LayoutParams>(child->getLayoutParams());
        if (!lp) {
            lp = std::make_shared<LayoutParams>(View::WRAP_CONTENT, View::WRAP_CONTENT);
            child->setLayoutParams(lp);
        }

        cassowary::ConstraintWidget* widget = mViewToWidget[child.get()];
        if (!widget) continue;

        if (lp->width == View::WRAP_CONTENT) {
            widget->setHorizontalDimensionBehaviour(cassowary::ConstraintWidget::DimensionBehaviour::WRAP_CONTENT);
        } else if (lp->width == View::MATCH_PARENT) {
            widget->setHorizontalDimensionBehaviour(cassowary::ConstraintWidget::DimensionBehaviour::MATCH_PARENT);
        } else if (lp->width == 0) {
            widget->setHorizontalDimensionBehaviour(cassowary::ConstraintWidget::DimensionBehaviour::MATCH_CONSTRAINT);
        } else {
            widget->setHorizontalDimensionBehaviour(cassowary::ConstraintWidget::DimensionBehaviour::FIXED);
        }

        if (lp->height == View::WRAP_CONTENT) {
            widget->setVerticalDimensionBehaviour(cassowary::ConstraintWidget::DimensionBehaviour::WRAP_CONTENT);
        } else if (lp->height == View::MATCH_PARENT) {
            widget->setVerticalDimensionBehaviour(cassowary::ConstraintWidget::DimensionBehaviour::MATCH_PARENT);
        } else if (lp->height == 0) {
            widget->setVerticalDimensionBehaviour(cassowary::ConstraintWidget::DimensionBehaviour::MATCH_CONSTRAINT);
        } else {
            widget->setVerticalDimensionBehaviour(cassowary::ConstraintWidget::DimensionBehaviour::FIXED);
        }

        int childWidthMeasureSpec = getChildMeasureSpec(widthMeasureSpec, 0, lp->width);
        int childHeightMeasureSpec = getChildMeasureSpec(heightMeasureSpec, 0, lp->height);

        child->measure(childWidthMeasureSpec, childHeightMeasureSpec);
        
        widget->setWidth(child->getMeasuredWidth());
        widget->setHeight(child->getMeasuredHeight());
    }

    // Solve constraints
    mLayoutWidget.layout();

    // Post-measure based on solved sizes
    int maxWidth = 0;
    int maxHeight = 0;
    
    for (auto& child : mChildren) {
        if (child->getVisibility() == View::GONE) {
            continue;
        }

        cassowary::ConstraintWidget* widget = mViewToWidget[child.get()];
        if (!widget) continue;

        int solvedWidth = widget->getWidth();
        int solvedHeight = widget->getHeight();
        
        // Re-measure child with EXACTLY if its bounds were constrained
        int childWidthSpec = View::makeMeasureSpec(solvedWidth, View::MEASURE_SPEC_EXACTLY);
        int childHeightSpec = View::makeMeasureSpec(solvedHeight, View::MEASURE_SPEC_EXACTLY);
        child->measure(childWidthSpec, childHeightSpec);

        maxWidth = std::max(maxWidth, widget->getX() + solvedWidth);
        maxHeight = std::max(maxHeight, widget->getY() + solvedHeight);
    }

    int measuredWidth = (widthMode == MEASURE_SPEC_EXACTLY) ? widthSize : maxWidth;
    int measuredHeight = (heightMode == MEASURE_SPEC_EXACTLY) ? heightSize : maxHeight;

    setMeasuredDimension(measuredWidth, measuredHeight);
}

void ConstraintLayout::onLayout(bool changed, int l, int t, int r, int b) {
    for (auto& child : mChildren) {
        if (child->getVisibility() == View::GONE) {
            continue;
        }
        cassowary::ConstraintWidget* widget = mViewToWidget[child.get()];
        if (!widget) continue;
        
        int x = widget->getX();
        int y = widget->getY();
        int w = widget->getWidth();
        int h = widget->getHeight();
        
        child->layout(x, y, x + w, y + h);
    }
}

std::shared_ptr<View::LayoutParams> ConstraintLayout::generateLayoutParams(android::ResXMLParser* parser) {
    auto lp = std::make_shared<LayoutParams>(View::WRAP_CONTENT, View::WRAP_CONTENT);
    if (!parser) return lp;
    ViewGroup::parseBaseLayoutParams(lp, parser);

    auto parseConstraintTarget = [&](uint8_t type, uint32_t data, const std::string& rawValue) {
        if (type == android::Res_value::TYPE_STRING) {
            if (rawValue == "parent") return 0;
        }
        if (data == 0) return 0;
        return (int)data;
    };

    size_t tagLen;
    const char16_t* tag16 = parser->getElementName(&tagLen);
    if (tag16) {
        std::string tagStr = android::util::Utf16ToUtf8(android::StringPiece16(tag16, tagLen));
        if (tagStr.find("Guideline") != std::string::npos) lp->isGuideline = true;
    }

    for (size_t i = 0; i < parser->getAttributeCount(); i++) {
        size_t nameLen;
        const char16_t* name16 = parser->getAttributeName(i, &nameLen);
        std::string attrName = name16 ? android::util::Utf16ToUtf8(android::StringPiece16(name16, nameLen)) : "";
        
        std::string rawValue = "";
        size_t valLen;
        const char16_t* val16 = parser->getAttributeStringValue(i, &valLen);
        if (val16) rawValue = android::util::Utf16ToUtf8(android::StringPiece16(val16, valLen));
        
        uint8_t type = parser->getAttributeDataType(i);
        uint32_t data = parser->getAttributeData(i);

        if (attrName == "layout_constraintTop_toTopOf") lp->topToTop = parseConstraintTarget(type, data, rawValue);
        else if (attrName == "layout_constraintTop_toBottomOf") lp->topToBottom = parseConstraintTarget(type, data, rawValue);
        else if (attrName == "layout_constraintBottom_toTopOf") lp->bottomToTop = parseConstraintTarget(type, data, rawValue);
        else if (attrName == "layout_constraintBottom_toBottomOf") lp->bottomToBottom = parseConstraintTarget(type, data, rawValue);
        else if (attrName == "layout_constraintStart_toStartOf" || attrName == "layout_constraintLeft_toLeftOf") lp->startToStart = parseConstraintTarget(type, data, rawValue);
        else if (attrName == "layout_constraintStart_toEndOf" || attrName == "layout_constraintLeft_toRightOf") lp->startToEnd = parseConstraintTarget(type, data, rawValue);
        else if (attrName == "layout_constraintEnd_toStartOf" || attrName == "layout_constraintRight_toLeftOf") lp->endToStart = parseConstraintTarget(type, data, rawValue);
        else if (attrName == "layout_constraintEnd_toEndOf" || attrName == "layout_constraintRight_toRightOf") lp->endToEnd = parseConstraintTarget(type, data, rawValue);
        else if (attrName == "layout_constraintHorizontal_bias") {
            union { uint32_t i; float f; } u;
            u.i = data;
            lp->horizontalBias = u.f;
        }
        else if (attrName == "layout_constraintVertical_bias") {
            union { uint32_t i; float f; } u;
            u.i = data;
            lp->verticalBias = u.f;
        }
        else if (attrName == "layout_constraintGuide_percent") {
            union { uint32_t i; float f; } u;
            u.i = data;
            lp->guidePercent = u.f;
        }
        else if (attrName == "layout_constraintGuide_begin") lp->guideBegin = (data >> 8) * 2;
        else if (attrName == "layout_constraintGuide_end") lp->guideEnd = (data >> 8) * 2;
        else if (attrName == "orientation") lp->orientation = data;
    }
    return lp;
}

} // namespace view
} // namespace setu