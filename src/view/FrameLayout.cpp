#include "FrameLayout.h"
#include <algorithm>

namespace windroid {
namespace view {

void FrameLayout::onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
    int maxWidth = 0;
    int maxHeight = 0;

    int widthMode = getMode(widthMeasureSpec);
    int widthSize = getSize(widthMeasureSpec);
    int heightMode = getMode(heightMeasureSpec);
    int heightSize = getSize(heightMeasureSpec);

    for (auto& child : mChildren) {
        auto lp = std::dynamic_pointer_cast<LayoutParams>(child->getLayoutParams());
        if (!lp) lp = std::make_shared<LayoutParams>(View::WRAP_CONTENT, View::WRAP_CONTENT);

        int childWidthSpec = 0;
        int childHeightSpec = 0;

        if (lp->width == View::MATCH_PARENT) {
            childWidthSpec = View::makeMeasureSpec(std::max(0, widthSize - lp->leftMargin - lp->rightMargin), View::MEASURE_SPEC_EXACTLY);
        } else if (lp->width == View::WRAP_CONTENT) {
            childWidthSpec = View::makeMeasureSpec(std::max(0, widthSize - lp->leftMargin - lp->rightMargin), View::MEASURE_SPEC_AT_MOST);
        } else {
            childWidthSpec = View::makeMeasureSpec(lp->width, View::MEASURE_SPEC_EXACTLY);
        }

        if (lp->height == View::MATCH_PARENT) {
            childHeightSpec = View::makeMeasureSpec(std::max(0, heightSize - lp->topMargin - lp->bottomMargin), View::MEASURE_SPEC_EXACTLY);
        } else if (lp->height == View::WRAP_CONTENT) {
            childHeightSpec = View::makeMeasureSpec(std::max(0, heightSize - lp->topMargin - lp->bottomMargin), View::MEASURE_SPEC_AT_MOST);
        } else {
            childHeightSpec = View::makeMeasureSpec(lp->height, View::MEASURE_SPEC_EXACTLY);
        }

        child->measure(childWidthSpec, childHeightSpec);

        maxWidth = std::max(maxWidth, child->getMeasuredWidth() + lp->leftMargin + lp->rightMargin);
        maxHeight = std::max(maxHeight, child->getMeasuredHeight() + lp->topMargin + lp->bottomMargin);
    }

    int measuredWidth = (widthMode == MEASURE_SPEC_EXACTLY) ? widthSize : maxWidth;
    int measuredHeight = (heightMode == MEASURE_SPEC_EXACTLY) ? heightSize : maxHeight;

    if (widthMode == MEASURE_SPEC_AT_MOST) measuredWidth = std::min(measuredWidth, widthSize);
    if (heightMode == MEASURE_SPEC_AT_MOST) measuredHeight = std::min(measuredHeight, heightSize);

    setMeasuredDimension(measuredWidth, measuredHeight);
}

void FrameLayout::onLayout(bool changed, int l, int t, int r, int b) {
    int parentWidth = r - l;
    int parentHeight = b - t;

    for (auto& child : mChildren) {
        auto lp = std::dynamic_pointer_cast<LayoutParams>(child->getLayoutParams());
        if (!lp) lp = std::make_shared<LayoutParams>(View::WRAP_CONTENT, View::WRAP_CONTENT);

        int cw = child->getMeasuredWidth();
        int ch = child->getMeasuredHeight();

        // Default top-left gravity for now.
        // A true implementation would check lp->gravity
        int childLeft = lp->leftMargin;
        int childTop = lp->topMargin;

        // Example: if gravity is CENTER
        if (lp->gravity == 17) { // Android Gravity.CENTER
            childLeft = (parentWidth - cw) / 2 + lp->leftMargin - lp->rightMargin;
            childTop = (parentHeight - ch) / 2 + lp->topMargin - lp->bottomMargin;
        }

        child->layout(childLeft, childTop, childLeft + cw, childTop + ch);
    }
}

} // namespace view
} // namespace windroid
