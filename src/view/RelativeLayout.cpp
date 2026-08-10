#include "RelativeLayout.h"
#include <algorithm>

namespace windroid {
namespace view {

const std::string RelativeLayout::LayoutParams::ABOVE = "layout_above";
const std::string RelativeLayout::LayoutParams::BELOW = "layout_below";
const std::string RelativeLayout::LayoutParams::LEFT_OF = "layout_toLeftOf";
const std::string RelativeLayout::LayoutParams::RIGHT_OF = "layout_toRightOf";
const std::string RelativeLayout::LayoutParams::ALIGN_PARENT_LEFT = "layout_alignParentLeft";
const std::string RelativeLayout::LayoutParams::ALIGN_PARENT_TOP = "layout_alignParentTop";
const std::string RelativeLayout::LayoutParams::ALIGN_PARENT_RIGHT = "layout_alignParentRight";
const std::string RelativeLayout::LayoutParams::ALIGN_PARENT_BOTTOM = "layout_alignParentBottom";

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

    int widthMode = getMode(widthMeasureSpec);
    int widthSize = getSize(widthMeasureSpec);
    int heightMode = getMode(heightMeasureSpec);
    int heightSize = getSize(heightMeasureSpec);

    // Naive pass 1: Measure all children without dependencies first
    for (auto& child : mChildren) {
        auto lp = std::dynamic_pointer_cast<LayoutParams>(child->getLayoutParams());
        if (!lp) lp = std::make_shared<LayoutParams>(View::WRAP_CONTENT, View::WRAP_CONTENT);

        int childWidthSpec = View::makeMeasureSpec(widthSize, (lp->width == View::MATCH_PARENT) ? View::MEASURE_SPEC_EXACTLY : View::MEASURE_SPEC_AT_MOST);
        int childHeightSpec = View::makeMeasureSpec(heightSize, (lp->height == View::MATCH_PARENT) ? View::MEASURE_SPEC_EXACTLY : View::MEASURE_SPEC_AT_MOST);
        if (lp->width > 0) childWidthSpec = View::makeMeasureSpec(lp->width, View::MEASURE_SPEC_EXACTLY);
        if (lp->height > 0) childHeightSpec = View::makeMeasureSpec(lp->height, View::MEASURE_SPEC_EXACTLY);

        child->measure(childWidthSpec, childHeightSpec);
    }

    // Measure self
    int measuredWidth = (widthMode == MEASURE_SPEC_EXACTLY) ? widthSize : widthSize; // Default to size
    int measuredHeight = (heightMode == MEASURE_SPEC_EXACTLY) ? heightSize : heightSize;
    setMeasuredDimension(measuredWidth, measuredHeight);
}

void RelativeLayout::onLayout(bool changed, int l, int t, int r, int b) {
    int parentWidth = r - l;
    int parentHeight = b - t;

    // Apply rules to figure out positioning
    for (auto& child : mChildren) {
        auto lp = std::dynamic_pointer_cast<LayoutParams>(child->getLayoutParams());
        if (!lp) continue;

        int cw = child->getMeasuredWidth();
        int ch = child->getMeasuredHeight();
        
        int childLeft = lp->leftMargin;
        int childTop = lp->topMargin;
        int childRight = childLeft + cw;
        int childBottom = childTop + ch;

        // Apply align parent rules
        if (lp->rules.count(LayoutParams::ALIGN_PARENT_RIGHT)) {
            childRight = parentWidth - lp->rightMargin;
            childLeft = childRight - cw;
        }
        if (lp->rules.count(LayoutParams::ALIGN_PARENT_BOTTOM)) {
            childBottom = parentHeight - lp->bottomMargin;
            childTop = childBottom - ch;
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

} // namespace view
} // namespace windroid
