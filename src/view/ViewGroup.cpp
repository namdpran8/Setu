#include "ViewGroup.h"
#include <algorithm>
#include "MotionEvent.h"
#include "../AxmlPraserer/AxmlParser.h"

namespace windroid {
namespace view {

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

void ViewGroup::parseBaseLayoutParams(std::shared_ptr<View::LayoutParams> lp, const AxmlNode* node) {
    if (!node || !lp) return;
    for (const auto& attr : node->attributes) {
        if (attr.name == "layout_width") {
            if (attr.typedValueType == 0x03) { // STRING
                if (attr.rawValue == "match_parent" || attr.rawValue == "fill_parent") lp->width = View::MATCH_PARENT;
                else if (attr.rawValue == "wrap_content") lp->width = View::WRAP_CONTENT;
            } else if (attr.typedValueType == 0x10 || attr.typedValueType == 0x11) { // INT
                if ((int)attr.typedValueData == -1) lp->width = View::MATCH_PARENT;
                else if ((int)attr.typedValueData == -2) lp->width = View::WRAP_CONTENT;
                else lp->width = attr.typedValueData;
            } else if (attr.typedValueType == 0x05) { // DIMENSION
                lp->width = attr.typedValueData; // Raw for now, should map px
            }
        } else if (attr.name == "layout_height") {
            if (attr.typedValueType == 0x03) { 
                if (attr.rawValue == "match_parent" || attr.rawValue == "fill_parent") lp->height = View::MATCH_PARENT;
                else if (attr.rawValue == "wrap_content") lp->height = View::WRAP_CONTENT;
            } else if (attr.typedValueType == 0x10 || attr.typedValueType == 0x11) { // INT
                if ((int)attr.typedValueData == -1) lp->height = View::MATCH_PARENT;
                else if ((int)attr.typedValueData == -2) lp->height = View::WRAP_CONTENT;
                else lp->height = attr.typedValueData;
            } else if (attr.typedValueType == 0x05) { // DIMENSION
                lp->height = attr.typedValueData;
            }
        } else if (attr.name == "layout_marginLeft" || attr.name == "layout_marginStart") {
            lp->leftMargin = attr.typedValueData;
        } else if (attr.name == "layout_marginRight" || attr.name == "layout_marginEnd") {
            lp->rightMargin = attr.typedValueData;
        } else if (attr.name == "layout_marginTop") {
            lp->topMargin = attr.typedValueData;
        } else if (attr.name == "layout_marginBottom") {
            lp->bottomMargin = attr.typedValueData;
        }
    }
}

std::shared_ptr<View::LayoutParams> ViewGroup::generateLayoutParams(const AxmlNode* node) {
    auto lp = std::make_shared<View::LayoutParams>(View::WRAP_CONTENT, View::WRAP_CONTENT);
    parseBaseLayoutParams(lp, node);
    return lp;
}

void ViewGroup::dump(int depth) {
    View::dump(depth);
    for (auto& child : mChildren) {
        child->dump(depth + 1);
    }
}

} // namespace view
} // namespace windroid
