#include "OverlayPanelLayout.h"

#include <algorithm>

namespace setu {
namespace view {

void OverlayPanelLayout::onMeasure(int widthMeasureSpec, int heightMeasureSpec) {
    const int widthMode = getMode(widthMeasureSpec);
    const int widthSize = getSize(widthMeasureSpec);
    const int heightMode = getMode(heightMeasureSpec);
    const int heightSize = getSize(heightMeasureSpec);

    int desiredWidth = 0;
    int desiredHeight = 0;
    for (const auto& child : mChildren) {
        if (!child || child->getVisibility() == View::GONE) continue;
        measureChildWithMargins(child, widthMeasureSpec, 0, heightMeasureSpec, 0);
        const auto lp = child->getLayoutParams();
        if (lp) {
            desiredWidth = std::max(desiredWidth, child->getMeasuredWidth() + lp->leftMargin + lp->rightMargin);
            desiredHeight = std::max(desiredHeight, child->getMeasuredHeight() + lp->topMargin + lp->bottomMargin);
        }
    }

    const int measuredWidth = widthMode == View::MEASURE_SPEC_EXACTLY ? widthSize :
        (widthMode == View::MEASURE_SPEC_AT_MOST ? std::min(desiredWidth, widthSize) : desiredWidth);
    const int measuredHeight = heightMode == View::MEASURE_SPEC_EXACTLY ? heightSize :
        (heightMode == View::MEASURE_SPEC_AT_MOST ? std::min(desiredHeight, heightSize) : desiredHeight);
    setMeasuredDimension(measuredWidth, measuredHeight);
}

void OverlayPanelLayout::onLayout(bool changed, int l, int t, int r, int b) {
    const int parentWidth = r - l;
    const int parentHeight = b - t;
    for (const auto& child : mChildren) {
        if (!child || child->getVisibility() == View::GONE) continue;
        const auto lp = child->getLayoutParams();
        const int left = lp ? lp->leftMargin : 0;
        const int top = lp ? lp->topMargin : 0;
        const int right = std::max(left, parentWidth - (lp ? lp->rightMargin : 0));
        const int bottom = std::max(top, parentHeight - (lp ? lp->bottomMargin : 0));
        child->layout(left, top, right, bottom);
    }
}

} // namespace view
} // namespace setu
