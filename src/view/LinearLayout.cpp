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
#include "LinearLayout.h"
#include <algorithm>
#include "androidfw/ResourceTypes.h"

namespace setu {
namespace view {

void LinearLayout::onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
    int widthMode = getMode(widthMeasureSpec);
    int heightMode = getMode(heightMeasureSpec);
    
    bool isVertical = (mOrientation == Orientation::VERTICAL);
    
    float totalWeight = 0.0f;
    int usedSize = 0;
    int maxOrthogonal = 0;
    // Space taken in this pass by children that are sized from excess space alone
    // (0 along the layout axis plus a weight). Those children have no intrinsic
    // size of their own, so the space is credited back to the excess pool before
    // distribution (AOSP's consumedExcessSpace).
    int consumedExcessSpace = 0;
    bool hasExcessOnlyChildren = false;

    for (auto& child : mChildren) {
        if (child->getVisibility() == View::GONE) continue;

        auto lp = std::dynamic_pointer_cast<LayoutParams>(child->getLayoutParams());
        if (!lp) lp = std::make_shared<LayoutParams>(View::WRAP_CONTENT, View::WRAP_CONTENT);

        int childWidthSpec, childHeightSpec;
        bool useExcessSpace = false;

        if (isVertical) {
            childWidthSpec = ViewGroup::getChildMeasureSpec(widthMeasureSpec, mPaddingLeft + mPaddingRight + lp->leftMargin + lp->rightMargin, lp->width);
            if (lp->weight > 0) {
                totalWeight += lp->weight;
                useExcessSpace = (lp->height == 0);
                childHeightSpec = View::makeMeasureSpec(0, View::MEASURE_SPEC_UNSPECIFIED);
            } else {
                childHeightSpec = ViewGroup::getChildMeasureSpec(heightMeasureSpec, mPaddingTop + mPaddingBottom + lp->topMargin + lp->bottomMargin, lp->height);
            }
        } else {
            childHeightSpec = ViewGroup::getChildMeasureSpec(heightMeasureSpec, mPaddingTop + mPaddingBottom + lp->topMargin + lp->bottomMargin, lp->height);
            if (lp->weight > 0) {
                totalWeight += lp->weight;
                useExcessSpace = (lp->width == 0);
                childWidthSpec = View::makeMeasureSpec(0, View::MEASURE_SPEC_UNSPECIFIED);
            } else {
                childWidthSpec = ViewGroup::getChildMeasureSpec(widthMeasureSpec, mPaddingLeft + mPaddingRight + lp->leftMargin + lp->rightMargin, lp->width);
            }
        }

        child->measure(childWidthSpec, childHeightSpec);

        if (isVertical) {
            usedSize += child->getMeasuredHeight() + lp->topMargin + lp->bottomMargin;
            maxOrthogonal = std::max(maxOrthogonal, child->getMeasuredWidth() + lp->leftMargin + lp->rightMargin);
        } else {
            usedSize += child->getMeasuredWidth() + lp->leftMargin + lp->rightMargin;
            maxOrthogonal = std::max(maxOrthogonal, child->getMeasuredHeight() + lp->topMargin + lp->bottomMargin);
        }

        if (useExcessSpace) {
            consumedExcessSpace += isVertical ? child->getMeasuredHeight() : child->getMeasuredWidth();
            hasExcessOnlyChildren = true;
        }
    }
    
    if (totalWeight > 0.0f) {
        if (isVertical) {
            // Seed the excess pool from our resolved height, crediting back the space
            // the excess-only children took in the first pass. This may be negative,
            // in which case the weighted children shrink instead of growing.
            int totalLength = usedSize + mPaddingTop + mPaddingBottom;
            int remaining = resolveSize(totalLength, heightMeasureSpec) - totalLength + consumedExcessSpace;
            bool mustRemeasure = hasExcessOnlyChildren && heightMode == View::MEASURE_SPEC_EXACTLY;

            if (remaining != 0 || mustRemeasure) {
                float remainingWeight = totalWeight;
                usedSize = 0;

                for (auto& child : mChildren) {
                    if (child->getVisibility() == View::GONE) continue;
                    auto lp = std::dynamic_pointer_cast<LayoutParams>(child->getLayoutParams());
                    if (!lp) lp = std::make_shared<LayoutParams>(View::WRAP_CONTENT, View::WRAP_CONTENT);

                    if (lp->weight > 0) {
                        // Take this child's share out of what is still left and deduct it,
                        // so integer truncation cannot lose pixels across several children.
                        int share = (int)(lp->weight * remaining / remainingWeight);
                        remaining -= share;
                        remainingWeight -= lp->weight;

                        // A child with no size of its own along the axis is laid out from its
                        // share alone; any other child keeps the size it measured to in the
                        // first pass and gets the share added on top.
                        int childHeight = (lp->height == 0) ? share : (child->getMeasuredHeight() + share);

                        int childHeightSpec = View::makeMeasureSpec(std::max(0, childHeight), View::MEASURE_SPEC_EXACTLY);
                        int childWidthSpec = ViewGroup::getChildMeasureSpec(widthMeasureSpec, mPaddingLeft + mPaddingRight + lp->leftMargin + lp->rightMargin, lp->width);
                        child->measure(childWidthSpec, childHeightSpec);
                    }

                    // Re-accumulate from what the children actually measured to, not from
                    // the size we asked for.
                    usedSize += child->getMeasuredHeight() + lp->topMargin + lp->bottomMargin;
                    maxOrthogonal = std::max(maxOrthogonal, child->getMeasuredWidth() + lp->leftMargin + lp->rightMargin);
                }
            }
        } else {
            int totalLength = usedSize + mPaddingLeft + mPaddingRight;
            int remaining = resolveSize(totalLength, widthMeasureSpec) - totalLength + consumedExcessSpace;
            bool mustRemeasure = hasExcessOnlyChildren && widthMode == View::MEASURE_SPEC_EXACTLY;

            if (remaining != 0 || mustRemeasure) {
                float remainingWeight = totalWeight;
                usedSize = 0;

                for (auto& child : mChildren) {
                    if (child->getVisibility() == View::GONE) continue;
                    auto lp = std::dynamic_pointer_cast<LayoutParams>(child->getLayoutParams());
                    if (!lp) lp = std::make_shared<LayoutParams>(View::WRAP_CONTENT, View::WRAP_CONTENT);

                    if (lp->weight > 0) {
                        int share = (int)(lp->weight * remaining / remainingWeight);
                        remaining -= share;
                        remainingWeight -= lp->weight;

                        int childWidth = (lp->width == 0) ? share : (child->getMeasuredWidth() + share);

                        int childWidthSpec = View::makeMeasureSpec(std::max(0, childWidth), View::MEASURE_SPEC_EXACTLY);
                        int childHeightSpec = ViewGroup::getChildMeasureSpec(heightMeasureSpec, mPaddingTop + mPaddingBottom + lp->topMargin + lp->bottomMargin, lp->height);
                        child->measure(childWidthSpec, childHeightSpec);
                    }

                    usedSize += child->getMeasuredWidth() + lp->leftMargin + lp->rightMargin;
                    maxOrthogonal = std::max(maxOrthogonal, child->getMeasuredHeight() + lp->topMargin + lp->bottomMargin);
                }
            }
        }
    }
    
    int totalWidth = isVertical ? (mPaddingLeft + mPaddingRight + maxOrthogonal) : (mPaddingLeft + mPaddingRight + usedSize);
    int totalHeight = isVertical ? (mPaddingTop + mPaddingBottom + usedSize) : (mPaddingTop + mPaddingBottom + maxOrthogonal);
    
    int measuredWidth = resolveSize(totalWidth, widthMeasureSpec);
    int measuredHeight = resolveSize(totalHeight, heightMeasureSpec);
    
    setMeasuredDimension(measuredWidth, measuredHeight);
}

void LinearLayout::onLayout(bool changed, int l, int t, int r, int b) {
    int childTop = mPaddingTop;
    int childLeft = mPaddingLeft;
    int width = r - l;
    int height = b - t;
    
    for (auto& child : mChildren) {
        if (child->getVisibility() == View::GONE) continue;
        
        auto lp = std::dynamic_pointer_cast<LayoutParams>(child->getLayoutParams());
        if (!lp) lp = std::make_shared<LayoutParams>(View::WRAP_CONTENT, View::WRAP_CONTENT);
        
        int cw = child->getMeasuredWidth();
        int ch = child->getMeasuredHeight();
        
        int gravity = lp->gravity >= 0 ? lp->gravity : mGravity;
        
        int childLeftFinal = childLeft + lp->leftMargin;
        int childTopFinal = childTop + lp->topMargin;
        
        if (mOrientation == Orientation::VERTICAL) {
            int availWidth = width - mPaddingLeft - mPaddingRight - lp->leftMargin - lp->rightMargin;
            int horizontalGravity = gravity & 0x07;
            if (horizontalGravity == 0x01) { // CENTER_HORIZONTAL
                childLeftFinal += (availWidth - cw) / 2;
            } else if (horizontalGravity == 0x05) { // RIGHT
                childLeftFinal += availWidth - cw;
            }
            
            child->layout(childLeftFinal, childTopFinal, childLeftFinal + cw, childTopFinal + ch);
            childTop = childTopFinal + ch + lp->bottomMargin;
        } else {
            int availHeight = height - mPaddingTop - mPaddingBottom - lp->topMargin - lp->bottomMargin;
            int verticalGravity = gravity & 0x70;
            if (verticalGravity == 0x10) { // CENTER_VERTICAL
                childTopFinal += (availHeight - ch) / 2;
            } else if (verticalGravity == 0x50) { // BOTTOM
                childTopFinal += availHeight - ch;
            }
            
            child->layout(childLeftFinal, childTopFinal, childLeftFinal + cw, childTopFinal + ch);
            childLeft = childLeftFinal + cw + lp->rightMargin;
        }
    }
}

std::shared_ptr<View::LayoutParams> LinearLayout::generateLayoutParams(android::ResXMLParser* parser) {
    auto lp = std::make_shared<LayoutParams>(View::WRAP_CONTENT, View::WRAP_CONTENT);
    if (!parser) return lp;
    ViewGroup::parseBaseLayoutParams(lp, parser);
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

        if (attrName == "layout_weight") {
            if (type == 0x04) { // FLOAT
                union { uint32_t i; float f; } u;
                u.i = data;
                lp->weight = u.f;
            } else if (type == 0x03 && val16) { // STRING
                try { lp->weight = std::stof(rawValue); } catch(...) {}
            } else {
                lp->weight = (float)data;
            }
        } else if (attrName == "layout_gravity") {
            if (type == 0x03 && val16) {
                int g = 0;
                if (rawValue.find("center") != std::string::npos) g |= 0x11;
                if (rawValue.find("bottom") != std::string::npos) g |= 0x50;
                if (rawValue.find("right") != std::string::npos || rawValue.find("end") != std::string::npos) g |= 0x05;
                lp->gravity = g;
            } else {
                lp->gravity = data;
            }
        }
    }
    return lp;
}

} // namespace view
} // namespace setu




