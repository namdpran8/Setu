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
#include "RelativeLayout.h"
#include <algorithm>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include "androidfw/ResourceTypes.h"

namespace setu {
namespace view {

static std::vector<std::shared_ptr<View>> getSortedChildren(const std::vector<std::shared_ptr<View>>& children) {
    std::unordered_map<View*, std::vector<std::shared_ptr<View>>> graph;
    std::unordered_map<View*, int> inDegree;
    std::vector<std::shared_ptr<View>> sorted;
    std::unordered_map<int, std::shared_ptr<View>> idMap;

    for (const auto& child : children) {
        if (child->getId() != 0) {
            idMap[child->getId()] = child;
        }
        inDegree[child.get()] = 0;
    }

    for (const auto& child : children) {
        auto lp = std::dynamic_pointer_cast<RelativeLayout::LayoutParams>(child->getLayoutParams());
        if (!lp) continue;

        for (const auto& rule : {RelativeLayout::LayoutParams::ABOVE, RelativeLayout::LayoutParams::BELOW, 
                                 RelativeLayout::LayoutParams::LEFT_OF, RelativeLayout::LayoutParams::RIGHT_OF}) {
            if (lp->rules.count(rule)) {
                int targetId = lp->rules[rule];
                if (idMap.count(targetId)) {
                    auto target = idMap[targetId];
                    graph[target.get()].push_back(child);
                    inDegree[child.get()]++;
                }
            }
        }
    }

    std::vector<std::shared_ptr<View>> queue;
    for (const auto& child : children) {
        if (inDegree[child.get()] == 0) {
            queue.push_back(child);
        }
    }

    while (!queue.empty()) {
        auto curr = queue.front();
        queue.erase(queue.begin());
        sorted.push_back(curr);

        for (const auto& dependent : graph[curr.get()]) {
            inDegree[dependent.get()]--;
            if (inDegree[dependent.get()] == 0) {
                queue.push_back(dependent);
            }
        }
    }

    if (sorted.size() != children.size()) {
        for (const auto& child : children) {
            if (std::find(sorted.begin(), sorted.end(), child) == sorted.end()) {
                sorted.push_back(child);
            }
        }
    }
    return sorted;
}

const std::string RelativeLayout::LayoutParams::ABOVE = "layout_above";
const std::string RelativeLayout::LayoutParams::BELOW = "layout_below";
const std::string RelativeLayout::LayoutParams::LEFT_OF = "layout_toLeftOf";
const std::string RelativeLayout::LayoutParams::RIGHT_OF = "layout_toRightOf";
const std::string RelativeLayout::LayoutParams::ALIGN_PARENT_LEFT = "layout_alignParentLeft";
const std::string RelativeLayout::LayoutParams::ALIGN_PARENT_TOP = "layout_alignParentTop";
const std::string RelativeLayout::LayoutParams::ALIGN_PARENT_RIGHT = "layout_alignParentRight";
const std::string RelativeLayout::LayoutParams::ALIGN_PARENT_BOTTOM = "layout_alignParentBottom";
const std::string RelativeLayout::LayoutParams::CENTER_IN_PARENT = "layout_centerInParent";
const std::string RelativeLayout::LayoutParams::CENTER_HORIZONTAL = "layout_centerHorizontal";
const std::string RelativeLayout::LayoutParams::CENTER_VERTICAL = "layout_centerVertical";

std::shared_ptr<View> RelativeLayout::getViewById(int id) {
    for (auto& child : mChildren) {
        if (child->getId() == id) {
            return child;
        }
    }
    return nullptr;
}

void RelativeLayout::onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
    int maxWidth = 0;
    int maxHeight = 0;

    int widthSize = getSize(widthMeasureSpec);
    int heightSize = getSize(heightMeasureSpec);
    auto content = getContentBounds(widthSize, heightSize);
    int horizontalPadding = widthSize - content.getWidth();
    int verticalPadding = heightSize - content.getHeight();

    // Topological sort pass
    auto sortedChildren = getSortedChildren(mChildren);

    for (auto& child : sortedChildren) {
        auto lp = std::dynamic_pointer_cast<LayoutParams>(child->getLayoutParams());
        if (!lp) lp = std::make_shared<LayoutParams>(View::WRAP_CONTENT, View::WRAP_CONTENT);

        int childWidthSpec = ViewGroup::getChildMeasureSpec(widthMeasureSpec,
            horizontalPadding + lp->leftMargin + lp->rightMargin, lp->width);
        int childHeightSpec = ViewGroup::getChildMeasureSpec(heightMeasureSpec,
            verticalPadding + lp->topMargin + lp->bottomMargin, lp->height);

        child->measure(childWidthSpec, childHeightSpec);
        maxWidth = std::max(maxWidth, child->getMeasuredWidth() + lp->leftMargin + lp->rightMargin);
        maxHeight = std::max(maxHeight, child->getMeasuredHeight() + lp->topMargin + lp->bottomMargin);
    }

    int measuredWidth = resolveSize(maxWidth + mPaddingLeft + mPaddingRight, widthMeasureSpec);
    int measuredHeight = resolveSize(maxHeight + mPaddingTop + mPaddingBottom, heightMeasureSpec);
    setMeasuredDimension(measuredWidth, measuredHeight);
}

void RelativeLayout::onLayout(bool changed, int l, int t, int r, int b) {
    auto content = getContentBounds(r - l, b - t);

    auto sortedChildren = getSortedChildren(mChildren);

    // Apply rules to figure out positioning
    for (auto& child : sortedChildren) {
        auto lp = std::dynamic_pointer_cast<LayoutParams>(child->getLayoutParams());
        if (!lp) continue;

        int cw = child->getMeasuredWidth();
        int ch = child->getMeasuredHeight();
        
        int childLeft = content.left + lp->leftMargin;
        int childTop = content.top + lp->topMargin;
        int childRight = childLeft + cw;
        int childBottom = childTop + ch;

        // Apply align parent rules
        if (lp->rules.count(LayoutParams::ALIGN_PARENT_LEFT)) {
            childLeft = content.left + lp->leftMargin;
            childRight = childLeft + cw;
        }
        if (lp->rules.count(LayoutParams::ALIGN_PARENT_TOP)) {
            childTop = content.top + lp->topMargin;
            childBottom = childTop + ch;
        }
        if (lp->rules.count(LayoutParams::ALIGN_PARENT_RIGHT)) {
            childRight = content.right - lp->rightMargin;
            childLeft = childRight - cw;
        }
        if (lp->rules.count(LayoutParams::ALIGN_PARENT_BOTTOM)) {
            childBottom = content.bottom - lp->bottomMargin;
            childTop = childBottom - ch;
        }
        if (lp->rules.count(LayoutParams::CENTER_IN_PARENT) ||
            lp->rules.count(LayoutParams::CENTER_HORIZONTAL)) {
            childLeft = content.left + (content.getWidth() - cw - lp->leftMargin - lp->rightMargin) / 2 + lp->leftMargin;
            childRight = childLeft + cw;
        }
        if (lp->rules.count(LayoutParams::CENTER_IN_PARENT) ||
            lp->rules.count(LayoutParams::CENTER_VERTICAL)) {
            childTop = content.top + (content.getHeight() - ch - lp->topMargin - lp->bottomMargin) / 2 + lp->topMargin;
            childBottom = childTop + ch;
        }

        // Apply relative positioning rules (very naive implementation without topological sort)
        if (lp->rules.count(LayoutParams::BELOW)) {
            auto target = getViewById(lp->rules[LayoutParams::BELOW]);
            if (target) {
                childTop = target->getBottom() + lp->topMargin;
                childBottom = childTop + ch;
            }
        }
        if (lp->rules.count(LayoutParams::ABOVE)) {
            auto target = getViewById(lp->rules[LayoutParams::ABOVE]);
            if (target) {
                childBottom = target->getTop() - lp->bottomMargin;
                childTop = childBottom - ch;
            }
        }
        if (lp->rules.count(LayoutParams::RIGHT_OF)) {
            auto target = getViewById(lp->rules[LayoutParams::RIGHT_OF]);
            if (target) {
                childLeft = target->getRight() + lp->leftMargin;
                childRight = childLeft + cw;
            }
        }
        if (lp->rules.count(LayoutParams::LEFT_OF)) {
            auto target = getViewById(lp->rules[LayoutParams::LEFT_OF]);
            if (target) {
                childRight = target->getLeft() - lp->rightMargin;
                childLeft = childRight - cw;
            }
        }

        child->layout(childLeft, childTop, childRight, childBottom);
    }
}

std::shared_ptr<View::LayoutParams> RelativeLayout::generateLayoutParams(android::ResXMLParser* parser) {
    auto lp = std::make_shared<LayoutParams>(View::WRAP_CONTENT, View::WRAP_CONTENT);
    if (!parser) return lp;
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

        if (attrName == LayoutParams::ABOVE ||
            attrName == LayoutParams::BELOW ||
            attrName == LayoutParams::LEFT_OF ||
            attrName == LayoutParams::RIGHT_OF ||
            attrName == LayoutParams::ALIGN_PARENT_LEFT ||
            attrName == LayoutParams::ALIGN_PARENT_TOP ||
            attrName == LayoutParams::ALIGN_PARENT_RIGHT ||
            attrName == LayoutParams::ALIGN_PARENT_BOTTOM ||
            attrName == LayoutParams::CENTER_IN_PARENT ||
            attrName == LayoutParams::CENTER_HORIZONTAL ||
            attrName == LayoutParams::CENTER_VERTICAL) {
            lp->rules[attrName] = data;
        }
    }
    return lp;
}

} // namespace view
} // namespace setu




