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
#include "ViewGroup.h"
#include <algorithm>
#include "MotionEvent.h"
#include "androidfw/ResourceTypes.h"
#include "../ui/XmlAttrs.h"


#include "../animation/LayoutTransition.h"

namespace setu {
namespace view {

void ViewGroup::dispatchAttachedToWindow() {
    View::dispatchAttachedToWindow();
    for (auto& child : mChildren) {
        child->dispatchAttachedToWindow();
    }
}

void ViewGroup::dispatchDetachedFromWindow() {
    View::dispatchDetachedFromWindow();
    for (auto& child : mChildren) {
        child->dispatchDetachedFromWindow();
    }
}

void ViewGroup::addView(std::shared_ptr<View> child) {
    if (child) {
        child->setParent(this);
    if (mAttachedToWindow) child->dispatchAttachedToWindow();
        mChildren.push_back(child);
        if (mLayoutTransition) {
            mLayoutTransition->addChild(this, child);
        }
    }
}

void ViewGroup::removeView(std::shared_ptr<View> child) {
    auto it = std::find(mChildren.begin(), mChildren.end(), child);
    if (it != mChildren.end()) {
        if (mLayoutTransition) {
            mDisappearingChildren.push_back(child);
            mLayoutTransition->removeChild(this, child);
        } else {
            if (mAttachedToWindow) (*it)->dispatchDetachedFromWindow();
        (*it)->setParent(nullptr);
        }
        mChildren.erase(it);
        invalidate();
    }
}

void ViewGroup::finishRemoveView(std::shared_ptr<View> child) {
    auto it = std::find(mDisappearingChildren.begin(), mDisappearingChildren.end(), child);
    if (it != mDisappearingChildren.end()) {
        if (mAttachedToWindow) (*it)->dispatchDetachedFromWindow();
        (*it)->setParent(nullptr);
        mDisappearingChildren.erase(it);
        invalidate();
    }
}


namespace {

// A TYPE_DIMENSION attribute is a fixed-point complex word, not a pixel count:
// 16dp compiles to 4097. This used to assign the raw word straight into a
// margin. Routed through the same decoder LayoutInflater and the drawable
// inflater use, so all three agree on what "16dp" means, and rounded the way
// AOSP's MarginLayoutParams does (getDimensionPixelSize, not truncation).
int layoutDimensionPx(uint32_t data) {
    return dimensionPixelSize(complexToDimensionPxWith(
        data, View::getDisplayDensity(), View::getScaledDensity()));
}

// A layout dimension attribute in pixels. False when the value is not one this
// layer can decode, and the caller then leaves its field at the default.
//
// The types that are *not* decodable matter more than the ones that are.
// android:layout_margin="@dimen/spacing" compiles to TYPE_REFERENCE, whose data
// word is a bare resource ID (0x7F040012), and a string dimension's data word is
// a string-pool index; neither is a pixel count. Turning either into one takes an
// AssetManager, which this layer deliberately cannot reach - XmlAttrs.h spells
// out why, and constraint_layout_test builds View/ViewGroup without
// ResourceManager to keep it that way.
//
// So the value is reported as absent rather than written through raw. That is
// what the callers below used to do, and a resource ID read as pixels is a
// two-billion-pixel margin: not a slightly-wrong layout but a collapsed one.
// Holding the default leaves the field for LayoutInflater::parseLayoutParams,
// which re-reads these same attributes with a ResourceManager in hand and does
// resolve the reference.
bool layoutDimensionAttr(uint8_t type, uint32_t data, int& out) {
    if (type == android::Res_value::TYPE_DIMENSION) {
        out = layoutDimensionPx(data);
        return true;
    }
    // A plain integer is already a pixel count.
    if (type == android::Res_value::TYPE_INT_DEC ||
        type == android::Res_value::TYPE_INT_HEX) {
        out = (int)data;
        return true;
    }
    return false;
}

} // namespace

std::shared_ptr<View> ViewGroup::findViewById(int targetId) {
    if (mId == targetId) return shared_from_this();
    for (auto& child : mChildren) {
        if (auto found = child->findViewById(targetId)) return found;
    }
    return nullptr;
}

void ViewGroup::onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
    // Base ViewGroup (used as a static container by LayoutInflater)
    // We shouldn't override children's measured dimensions here since
    // LayoutInflater already computed them statically.
    setMeasuredDimension(View::getSize(widthMeasureSpec), View::getSize(heightMeasureSpec));
}

void ViewGroup::onLayout(bool changed, int l, int t, int r, int b) {
    // We shouldn't force children to 0,0 here since LayoutInflater
    // already placed them at specific X,Y coordinates.
}

void ViewGroup::onDraw(graphics::Canvas& canvas) {
    // ViewGroup draws its own background (handled by View base class)
    // and then dispatches draw to children
    dispatchDraw(canvas);
}

void ViewGroup::dispatchDraw(graphics::Canvas& canvas) {
    for (auto& child : mChildren) {
        if (child->getVisibility() == View::VISIBLE) {
            child->updateRenderNode();
            canvas.drawRenderNode(child->getRenderNode());
        }
    }
    for (auto& child : mDisappearingChildren) {
        child->updateRenderNode();
        canvas.drawRenderNode(child->getRenderNode());
    }
}

bool ViewGroup::dispatchTouchEvent(MotionEvent& event) {
    float x = event.getX();
    float y = event.getY();

    // Iterate backwards for Z-ordering (top views first)
    for (auto it = mChildren.rbegin(); it != mChildren.rend(); ++it) {
        auto& child = *it;
        if (x >= child->getLeft() && x <= child->getRight() &&
            y >= child->getTop() && y <= child->getBottom()) {
            
            // Transform coordinates to child's local space
            float offsetX = -(float)child->getLeft();
            float offsetY = -(float)child->getTop();
            
            event.offsetLocation(offsetX, offsetY);
            
            if (child->dispatchTouchEvent(event)) {
                // Event handled by child
                event.offsetLocation(-offsetX, -offsetY); // Restore
                return true;
            }
            
            // Restore coordinates if not handled
            event.offsetLocation(-offsetX, -offsetY);
        }
    }

    // If no child handled it, try handling it ourselves
    return View::dispatchTouchEvent(event);
}

bool ViewGroup::dispatchKeyEvent(const KeyEvent& event) {
    // Traverse children to find the focused one that handles the event.
    // We rely on the child's own dispatchKeyEvent/onKeyEvent to check focus.
    for (auto it = mChildren.rbegin(); it != mChildren.rend(); ++it) {
        if ((*it)->dispatchKeyEvent(event)) return true;
    }
    // If no child handled it (or none focused), try handling it ourselves
    return View::dispatchKeyEvent(event);
}

int ViewGroup::getChildMeasureSpec(int spec, int padding, int childDimension) {
    int specMode = View::getMode(spec);
    int specSize = View::getSize(spec);
    int size = std::max(0, specSize - padding);

    int resultSize = 0;
    int resultMode = 0;

    if (specMode == View::MEASURE_SPEC_EXACTLY) {
        if (childDimension >= 0) {
            resultSize = childDimension;
            resultMode = View::MEASURE_SPEC_EXACTLY;
        } else if (childDimension == View::MATCH_PARENT) {
            resultSize = size;
            resultMode = View::MEASURE_SPEC_EXACTLY;
        } else if (childDimension == View::WRAP_CONTENT) {
            resultSize = size;
            resultMode = View::MEASURE_SPEC_AT_MOST;
        }
    } else if (specMode == View::MEASURE_SPEC_AT_MOST) {
        if (childDimension >= 0) {
            resultSize = childDimension;
            resultMode = View::MEASURE_SPEC_EXACTLY;
        } else if (childDimension == View::MATCH_PARENT) {
            resultSize = size;
            resultMode = View::MEASURE_SPEC_AT_MOST;
        } else if (childDimension == View::WRAP_CONTENT) {
            resultSize = size;
            resultMode = View::MEASURE_SPEC_AT_MOST;
        }
    } else if (specMode == View::MEASURE_SPEC_UNSPECIFIED) {
        if (childDimension >= 0) {
            resultSize = childDimension;
            resultMode = View::MEASURE_SPEC_EXACTLY;
        } else if (childDimension == View::MATCH_PARENT) {
            resultSize = size;
            resultMode = View::MEASURE_SPEC_UNSPECIFIED;
        } else if (childDimension == View::WRAP_CONTENT) {
            resultSize = size;
            resultMode = View::MEASURE_SPEC_UNSPECIFIED;
        }
    }
    return View::makeMeasureSpec(resultSize, resultMode);
}

void ViewGroup::measureChild(std::shared_ptr<View> child, int parentWidthMeasureSpec, int parentHeightMeasureSpec) {
    auto lp = child->getLayoutParams();
    if (!lp) return;

    int childWidthMeasureSpec = getChildMeasureSpec(parentWidthMeasureSpec, 0, lp->width);
    int childHeightMeasureSpec = getChildMeasureSpec(parentHeightMeasureSpec, 0, lp->height);

    child->measure(childWidthMeasureSpec, childHeightMeasureSpec);
}

void ViewGroup::measureChildWithMargins(std::shared_ptr<View> child, 
        int parentWidthMeasureSpec, int widthUsed,
        int parentHeightMeasureSpec, int heightUsed) {
    auto lp = child->getLayoutParams();
    if (!lp) return;

    int childWidthMeasureSpec = getChildMeasureSpec(parentWidthMeasureSpec,
            widthUsed + lp->leftMargin + lp->rightMargin, lp->width);
    int childHeightMeasureSpec = getChildMeasureSpec(parentHeightMeasureSpec,
            heightUsed + lp->topMargin + lp->bottomMargin, lp->height);

    child->measure(childWidthMeasureSpec, childHeightMeasureSpec);
}

void ViewGroup::parseBaseLayoutParams(std::shared_ptr<View::LayoutParams> lp, android::ResXMLParser* parser) {
    if (!parser || !lp) return;
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

        if (attrName == "layout_width") {
            if (type == 0x03) { // STRING
                if (rawValue == "match_parent" || rawValue == "fill_parent") lp->width = View::MATCH_PARENT;
                else if (rawValue == "wrap_content") lp->width = View::WRAP_CONTENT;
            } else if (type == 0x10 || type == 0x11) { // INT
                if ((int)data == -1) lp->width = View::MATCH_PARENT;
                else if ((int)data == -2) lp->width = View::WRAP_CONTENT;
                else lp->width = data;
            } else if (type == 0x05) { // DIMENSION
                lp->width = layoutDimensionPx(data);
            }
        } else if (attrName == "layout_height") {
            if (type == 0x03) {
                if (rawValue == "match_parent" || rawValue == "fill_parent") lp->height = View::MATCH_PARENT;
                else if (rawValue == "wrap_content") lp->height = View::WRAP_CONTENT;
            } else if (type == 0x10 || type == 0x11) { // INT
                if ((int)data == -1) lp->height = View::MATCH_PARENT;
                else if ((int)data == -2) lp->height = View::WRAP_CONTENT;
                else lp->height = data;
            } else if (type == 0x05) { // DIMENSION
                lp->height = layoutDimensionPx(data);
            }
        } else if (attrName == "layout_margin") {
            // Shorthand: sets all four edges. Was dropped entirely before, so a
            // layout_margin with no per-edge overrides produced no spacing at all.
            // Unlike the per-edge attributes below, no later pass in
            // LayoutInflater re-reads this one, so an undecodable value here just
            // means no margin.
            int m;
            if (layoutDimensionAttr(type, data, m)) {
                lp->leftMargin = lp->topMargin = lp->rightMargin = lp->bottomMargin = m;
            }
        } else if (attrName == "layout_marginLeft" || attrName == "layout_marginStart") {
            layoutDimensionAttr(type, data, lp->leftMargin);
        } else if (attrName == "layout_marginRight" || attrName == "layout_marginEnd") {
            layoutDimensionAttr(type, data, lp->rightMargin);
        } else if (attrName == "layout_marginTop") {
            layoutDimensionAttr(type, data, lp->topMargin);
        } else if (attrName == "layout_marginBottom") {
            layoutDimensionAttr(type, data, lp->bottomMargin);
        }
    }
}

std::shared_ptr<View::LayoutParams> ViewGroup::generateLayoutParams(android::ResXMLParser* parser) {
    auto lp = std::make_shared<View::LayoutParams>(View::WRAP_CONTENT, View::WRAP_CONTENT);
    parseBaseLayoutParams(lp, parser);
    return lp;
}

void ViewGroup::dump(int depth) {
    View::dump(depth);
    for (auto& child : mChildren) {
        child->dump(depth + 1);
    }
}

} // namespace view
} // namespace setu





